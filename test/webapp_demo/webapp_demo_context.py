from http.server import HTTPServer, BaseHTTPRequestHandler
import sys, subprocess, time, copy, cgi, os, signal, pprint
import simplejson as json
from io import BytesIO


class SimpleHTTPRequestHandler(BaseHTTPRequestHandler):
    def __init__(self, *arg, **kw):
        self.backend = kw["backend"]
        del kw["backend"]
        BaseHTTPRequestHandler.__init__(self, *arg, **kw)

    def create_frame(self, contents):
        return """
<html>
        <body>
        <form action="/" method="POST">
                <input type="hidden" name="type" value="restart">
                <input type="submit" value="restart">
        </form>
        <form action="/" method="POST">
                <input type="hidden" name="type" value="add-user">
                user: <input type="text" name="user">
                <input type="submit" value="add user">
        </form>
        <form action="/" method="POST">
                <input type="hidden" name="type" value="create-gradidos">
                user: <input type="text" name="user">
                amount: <input type="text" name="amount">
                <input type="submit" value="create gradidos">
        </form>
        <form action="/" method="POST">
                <input type="hidden" name="type" value="transfer">
                sender: <input type="text" name="sender">
                receiver: <input type="text" name="receiver">
                amount: <input type="text" name="amount">
                <input type="submit" value="transfer">
        </form>
        %s
        </body>
</html>
""" % contents

    def show_page(self, rpc_response=None):
        data_contents = "none"
        if rpc_response:
            data_contents = "<pre>%s</pre>" % str(rpc_response)
        else:
            zz = self.backend.get_bchain()
            # converting hex representation to text for better readability
            for i in zz:
                if "transaction" in i:
                    if "add_user" in i["transaction"]:
                        user = i["transaction"]["add_user"]["user"]
                        user = user.decode("hex")
                        i["transaction"]["add_user"]["user"] = user
                    if "gradido_creation" in i["transaction"]:
                        user = i["transaction"]["gradido_creation"]["user"]
                        user = user.decode("hex")
                        i["transaction"]["gradido_creation"]["user"] = user
                    if "local_transfer" in i["transaction"]:
                        user = i["transaction"]["local_transfer"]["sender"]["user"]
                        user = user.decode("hex")
                        i["transaction"]["local_transfer"]["sender"]["user"] = user
                        user = i["transaction"]["local_transfer"]["receiver"]["user"]
                        user = user.decode("hex")
                        i["transaction"]["local_transfer"]["receiver"]["user"] = user

            zz.reverse()
            data_contents = "<pre>%s</pre>" % json.dumps(zz, indent=4)
        rr = self.create_frame(data_contents);
        self.wfile.write(rr)

    def do_GET(self):
        self.send_response(200)
        self.send_header("Content-type", "text/html")
        self.end_headers()
        self.show_page()

    def do_POST(self):
        length = int(self.headers.getheader('content-length'))
        postvars = cgi.parse_qs(self.rfile.read(length), keep_blank_values=1)
        self.send_response(200)
        self.send_header("Content-type", "text/html")
        self.end_headers()
        if postvars["type"][0] == "restart":
            self.backend.stop();
        if postvars["type"][0] == "add-user":
            self.backend.add_user(postvars["user"][0])
        elif postvars["type"][0] == "create-gradidos":
            self.backend.create_gradidos(postvars["user"][0],
                                         float(postvars["amount"][0]))
        elif postvars["type"][0] == "transfer":
            self.backend.transfer(postvars["sender"][0],
                                  postvars["receiver"][0],
                                  float(postvars["amount"][0]))
        self.show_page()


class WebappDemo(object):
    def start(self, context):
        self.context = context
        self.topic_id = ".".join([str(i) for i in context.doc["test-config"]["hedera-topic-id"]])
        sys.stderr.write("starting eternal web demo\n")
        port = context.args[0]
        def create_handler(*arg, **kw):
            kw["backend"] = self
            return SimpleHTTPRequestHandler(*arg, **kw)
        httpd = HTTPServer(('0.0.0.0', port), create_handler)
        httpd.serve_forever()
    def get_bchain(self):
        bchain = subprocess.check_output(["../build/dump_blockchain", "test-stage/gradido-node-0/t_blocks.%s.bc" % self.topic_id], stderr=subprocess.STDOUT)
        return json.loads(bchain)

    def add_user(self, user):
        ii = self.context.path.as_arr()[1]
        sr = self.context.doc["steps"][ii]
        req = copy.deepcopy(sr["messages"]["hedera-message"])
        req["_arg"]["request"]["bodyBytes"] = copy.deepcopy(sr["messages"]["add-user"]["parts"]["message-body"])
        user_pubkey = user[:32]
        user_pubkey += "." * (32 - len(user_pubkey))
        req["_arg"]["request"]["bodyBytes"]["consensusSubmitMessage"][
            "message"]["body_bytes"]["group_member_update"][
                "user_pubkey"] = user_pubkey
        req["_arg"]["request"]["bodyBytes"]["transactionID"][
            "transactionValidStart"]["seconds"] = self.get_seconds_from_epoch(self.context)
        self.context.args = [[req]]
        initial = self.get_record_number()
        self.proc_man.do_hedera_calls(self.context)
        self.wait_for_update(initial)
    def create_gradidos(self, user, amount):
        ii = self.context.path.as_arr()[1]
        sr = self.context.doc["steps"][ii]
        req = copy.deepcopy(sr["messages"]["hedera-message"])
        req["_arg"]["request"]["bodyBytes"] = copy.deepcopy(sr["messages"]["create-gradidos"]["parts"]["message-body"])
        user_pubkey = user[:32]
        user_pubkey += "." * (32 - len(user_pubkey))
        req["_arg"]["request"]["bodyBytes"]["consensusSubmitMessage"][
            "message"]["body_bytes"]["creation"]["receiver"][
                "pubkey"] = user_pubkey
        req["_arg"]["request"]["bodyBytes"]["consensusSubmitMessage"][
            "message"]["body_bytes"]["creation"]["receiver"][
                "amount"] = amount
        req["_arg"]["request"]["bodyBytes"]["transactionID"][
            "transactionValidStart"]["seconds"] = self.get_seconds_from_epoch(self.context)
        self.context.args = [[req]]
        initial = self.get_record_number()
        self.proc_man.do_hedera_calls(self.context)
        self.wait_for_update(initial)

    def transfer(self, sender, receiver, amount):
        ii = self.context.path.as_arr()[1]
        sr = self.context.doc["steps"][ii]
        req = copy.deepcopy(sr["messages"]["hedera-message"])
        req["_arg"]["request"]["bodyBytes"] = copy.deepcopy(sr["messages"]["transfer"]["parts"]["message-body"])
        sender_pubkey = sender[:32]
        sender_pubkey += "." * (32 - len(sender_pubkey))
        receiver_pubkey = receiver[:32]
        receiver_pubkey += "." * (32 - len(receiver_pubkey))
        req["_arg"]["request"]["bodyBytes"]["consensusSubmitMessage"][
            "message"]["body_bytes"]["transfer"]["local"]["sender"][
                "pubkey"] = sender_pubkey
        req["_arg"]["request"]["bodyBytes"]["consensusSubmitMessage"][
            "message"]["body_bytes"]["transfer"]["local"]["sender"][
                "amount"] = amount
        req["_arg"]["request"]["bodyBytes"]["consensusSubmitMessage"][
            "message"]["body_bytes"]["transfer"]["local"][
                "receiver"] = receiver_pubkey
        req["_arg"]["request"]["bodyBytes"]["transactionID"][
            "transactionValidStart"]["seconds"] = self.get_seconds_from_epoch(self.context)
        self.context.args = [[req]]
        initial = self.get_record_number()
        self.proc_man.do_hedera_calls(self.context)
        self.wait_for_update(initial)
    def stop(self):
        self.cleanup()
        os.system('kill %d' % os.getpid())
    def get_record_number(self):
        res = subprocess.check_output(["../build/dump_blockchain", "test-stage/gradido-node-0/t_blocks.%s.bc" % self.topic_id, "-c"], stderr=subprocess.STDOUT)
        return int(res)
    def wait_for_update(self, initial):
        while (True):
            time.sleep(1)
            curr = self.get_record_number()
            if curr > initial:
                break

class AdvancedHTTPRequestHandler(BaseHTTPRequestHandler):
    def __init__(self, *arg, **kw):
        self.backend = kw["backend"]
        del kw["backend"]
        BaseHTTPRequestHandler.__init__(self, *arg, **kw)

    def create_frame(self, curr_node_id, curr_blockchain_id, 
                     node_list, blockchain_list, bchain_contents):
        is_started = self.backend.is_current_node_running()
        def get_disabled(bval):
            if bval:
                return ""
            else:
                return "disabled"
        tab_header = []
        for i in blockchain_list:
            active = (i == curr_blockchain_id)
            style = ""
            if not active:
                style = 'style="background-color: #dedede;"'
            th = """<input type="submit" name="tab-name" value="%s" %s>""" % (i, style)
            tab_header.append(th)

        tab = ""
        if not curr_blockchain_id:
            pass
        elif curr_blockchain_id == "group-register":
            tab = """
        <div style="background-color: #f2f2f2; width: 100%%; padding-top: 5px;">
        <form action="/" method="POST" class="node-agnostic">
                <input type="hidden" name="type" value="add-group">
                group_id: <input type="text" name="group-id">
                <input type="submit" value="add group">
        </form>
        </div>
            %s
""" % bchain_contents
        else:
            def is_selected_b(i):
                if i == curr_blockchain_id:
                    return """selected="selected" """
                else:
                    return ""
            only_ledgers = [i for i in blockchain_list if i != "group-register"]
            receiver_groups = ["""
                        <option value="%s" %s>%s</option>
            """ % (i, is_selected_b(i), i) for i in only_ledgers]
            

            tab = """
        <div style="background-color: #f2f2f2; width: 100%%; padding-top: 5px;">
        <form action="/" method="POST" class="node-specific">
                <input type="hidden" name="type" value="attach-blockchain-to-node">
                <input type="submit" value="attach">
        </form>
        <form action="/" method="POST" class="node-specific">
                <input type="hidden" name="type" value="rpc-get-users">
                <input type="submit" value="get users">
        </form>
        <form action="/" method="POST" class="node-agnostic">
                <input type="hidden" name="type" value="add-user">
                user: <input type="text" name="user">
                <input type="submit" value="add user" %(add-user-disabled)s>
        </form>
        <form action="/" method="POST" class="node-agnostic">
                <input type="hidden" name="type" value="create-gradidos">
                user: <input type="text" name="user">
                amount: <input type="text" name="amount">
                <input type="submit" value="create gradidos" %(create-gradidos-disabled)s>
        </form>
        <form action="/" method="POST" class="node-agnostic">
                <input type="hidden" name="type" value="transfer">
                sender: <input type="text" name="sender">
                receiver: 
                    <select name="receiver-bchain">
                    %(receiver_groups)s
                    </select>
                    <input type="text" name="receiver">
                amount: <input type="text" name="amount">
                <input type="submit" value="transfer" %(transfer-disabled)s>
        </form>
        </div>
        %(bchain_contents)s
""" % {"receiver_groups": receiver_groups,
                     "bchain_contents": bchain_contents,
                     "add-user-disabled": True,
                     "create-gradidos-disabled": True,
                     "transfer-disabled": True
        }
        tab_header = """
        <form action="/" method="POST" style="margin-bottom: 0;">
                <input type="hidden" name="type" value="change-tab">
                %s
        </form>
""" % "".join(tab_header)

        def is_selected(i):
            if i == curr_node_id:
                return """selected="selected" """
            else:
                return ""

        node_options = ["""
                        <option value="%s" %s>%s</option>
        """ % (i, is_selected(i), i) for i in node_list]
        node_options = "".join(node_options)

        return """
<html>
        <head>
                <style>
                        body {
                                margin: 0;
                        }
                        .contents { 
                                margin: 5px;
                        }
                        form.inline-form {
                                display: inline-block; 
                        }
                        .logo-strip {
                                background-color: #b7410e;
                                width: 100%%;
                                padding-left: 5px;
                                padding-right: 5px;
                                padding-top: 2px;
                                padding-bottom: 2px;
                                margin-bottom: 2px;
                        }
                        .logo {
                                font-color: #007D15;
                                color: white;
                                text-shadow:
                                -1px -1px 0 #000,
                                1px -1px 0 #000,
                                -1px 1px 0 #000,
                                1px 1px 0 #000;  
                        }
                        .node-specific {
                                background-color: #1B7CEA;
                                padding: 2px;
                                display: inline-block;
                        }
                        .node-agnostic {
                                background-color: #FDAE22;
                                padding: 2px;
                                display: inline-block;
                        }
                        .from-file-system {
                                padding: 2px;
                                border: 2px solid #A1B439;
                        }
                        .from-rpc {
                                padding: 2px;
                                border: 2px solid #F25822;
                        }
                </style>
        </head>
        <body>
        <div class="logo-strip">
                <span class="logo">Gradido</span>
                <span class="logo-2">cluster demo v3.0</span>
                <span style="float: right; padding-right: 10px;">
                <span style="color:#1B7CEA;">&#x25a0;</span>node specific
                <span style="color:#FDAE22;">&#x25a0;</span>node agnostic
                <span style="color:#A1B439;">&#x25a1;</span>from file system
                <span style="color:#F25822;">&#x25a1;</span>from RPC
        </span>
        </div>
        <div class="contents">
        <form action="/" method="POST" class="inline-form">
                <input type="hidden" name="type" value="change-curr-node">
                <select name="curr-node">
                %(node_options)s
                </select>
                <span class="node-specific">
                        <input type="submit" name="action" value="refresh" %(refresh-disabled)s>
                </span>
        </form>
        <form action="/" method="POST" class="inline-form node-agnostic">
                <input type="hidden" name="type" value="add-node">
                node_id: <input type="text" name="node-id">
                <input type="submit" name="action" value="add node">
        </form>
        <form action="/" method="POST" class="inline-form node-specific">
                <input type="hidden" name="type" value="start-curr-node">
                <input type="submit" name="action" value="start" %(start-disabled)s>
        </form>
        <form action="/" method="POST" class="inline-form node-specific">
                <input type="hidden" name="type" value="stop-curr-node">
                <input type="submit" name="action" value="stop" %(stop-disabled)s>
        </form>
        <form action="/" method="POST" class="inline-form node-agnostic">
                <input type="hidden" name="type" value="reset-all">
                <input type="submit" value="reset all">
        </form>

        %(tab_header)s
        %(tab)s
        </div>
        </body>
</html>
""" % {"node_options": node_options, 
       "tab_header": tab_header, 
       "tab": tab,
       "refresh-disabled": get_disabled(True),
       "start-disabled": get_disabled(bool(curr_node_id) and not
                                        is_started),
       "stop-disabled": get_disabled(bool(curr_node_id) and
                                        is_started)}



    def show_page(self, rpc_response=None):
        contents = "<pre class='from-file-system'>N/A</pre>"
        if rpc_response:
            contents = "<pre class='from-rpc'>%s</pre>" % str(rpc_response)
        else:
            zz = self.backend.get_bchain()
            if type(zz) == list:
                zz.reverse()
                contents = "<pre class='from-file-system'>%s</pre>" % json.dumps(zz, indent=4)

        # converting hex representation to text for better readability
        # for i in zz:
        #     if "transaction" in i:
        #         if "add_user" in i["transaction"]:
        #             user = i["transaction"]["add_user"]["user"]
        #             user = user.decode("hex")
        #             i["transaction"]["add_user"]["user"] = user
        #         if "gradido_creation" in i["transaction"]:
        #             user = i["transaction"]["gradido_creation"]["user"]
        #             user = user.decode("hex")
        #             i["transaction"]["gradido_creation"]["user"] = user
        #         if "local_transfer" in i["transaction"]:
        #             user = i["transaction"]["local_transfer"]["sender"]["user"]
        #             user = user.decode("hex")
        #             i["transaction"]["local_transfer"]["sender"]["user"] = user
        #             user = i["transaction"]["local_transfer"]["receiver"]["user"]
        #             user = user.decode("hex")
        #             i["transaction"]["local_transfer"]["receiver"]["user"] = user

        node_list = self.backend.get_node_list()
        blockchain_list = self.backend.get_bchain_list()

        node_id = self.backend.get_node_id()
        blockchain_id = self.backend.get_blockchain_id()

        rr = self.create_frame(node_id, 
                               blockchain_id,
                               node_list, blockchain_list, contents);
        self.wfile.write(rr)

    def do_GET(self):
        self.send_response(200)
        self.send_header("Content-type", "text/html")
        self.end_headers()
        self.show_page()

    def do_POST(self):
        length = int(self.headers.getheader('content-length'))
        postvars = cgi.parse_qs(self.rfile.read(length), keep_blank_values=1)
        self.send_response(200)
        self.send_header("Content-type", "text/html")
        self.end_headers()
        rt = postvars["type"][0]
        rpc_response = None
        if rt == "reset-all":
            self.backend.stop();
        elif rt == "attach-blockchain-to-node":
            self.backend.attach_blockchain()
        elif rt == "rpc-get-users":
            rpc_response = self.backend.rpc_get_users()
        elif rt == "add-user":
            self.backend.add_user(postvars["user"][0])
        elif rt == "create-gradidos":
            amount = postvars["amount"][0] and float(postvars["amount"][0]) or 0
            self.backend.create_gradidos(postvars["user"][0],
                                         amount)
        elif rt == "transfer":
            self.backend.transfer(postvars["sender"][0],
                                  postvars["receiver"][0],
                                  float(postvars["amount"][0]),
                                  postvars["receiver-bchain"][0]
            )

        elif rt == "add-group":
            self.backend.add_group(postvars["group-id"][0])
        elif rt == "add-node":
            self.backend.add_node(postvars["node-id"][0])
        elif rt == "change-curr-node":
            self.backend.set_node_id(postvars["curr-node"][0])
        elif rt == "start-curr-node":
            self.backend.start_curr_node()
        elif rt == "stop-curr-node":
            self.backend.stop_curr_node()
        elif rt == "change-tab":
            self.backend.set_blockchain_id(postvars["tab-name"][0])
        elif rt == "refresh":
            pass

        self.show_page(rpc_response)

    
class WebappDemoFull(object):
    def __init__(self, proc_man):
        self.node_id = None
        self.blockchain_id = None
        self.node_list = []
        self.node_proc_ids = {}
        self.node_current_endpoint_port = 13000
        self.node_grpc_endpoint = {}
        self.blockchains = {}
        self.blockchain_list = []
        self.topic_id_pool = []
        self.proc_man = proc_man

    def signal_node_to_reload(self, node_id):
        if node_id in self.node_proc_ids:
            pid = self.proc_man.get_pid(node_id)
            os.kill(pid, signal.SIGUSR1)

    def get_tid_str(self, tid):
        return "%d.%d.%d" % tid

    def get_node_id(self):
        return self.node_id
    def get_blockchain_id(self):
        return self.blockchain_id
    def set_node_id(self, node_id):
        self.node_id = node_id
    def set_blockchain_id(self, blockchain_id):
        self.blockchain_id = blockchain_id
    def get_bchain(self):
        if not self.blockchain_id:
            return []
        dumper = self.blockchain_id == "group-register" and "dump_group_blockchain" or "dump_blockchain"
        folder = "test-stage/%s/%s.%s.bc" % (
            self.node_id, self.blockchain_id, 
            self.get_tid_str(self.blockchains[self.blockchain_id]))

        if os.path.isdir(folder):
            bchain = subprocess.check_output(["../build/%s" % dumper, 
                                              folder], 
                                             stderr=subprocess.STDOUT)
            return json.loads(bchain)
        else:
            return False
    def get_bchain_list(self):
        if not self.node_id:
            return []
        return copy.deepcopy(self.blockchain_list)

    def set_topic_id(self, req, tid):
        req["_arg"]["request"]["bodyBytes"]["consensusSubmitMessage"][
            "topicID"]["shardNum"] = tid[0]
        req["_arg"]["request"]["bodyBytes"]["consensusSubmitMessage"][
            "topicID"]["realmNum"] = tid[1]
        req["_arg"]["request"]["bodyBytes"]["consensusSubmitMessage"][
            "topicID"]["topicNum"] = tid[2]

    def set_transaction_id(self, req):
        z = self.proc_man.get_timestamp_from_epoch(self.context)
        req["_arg"]["request"]["bodyBytes"]["transactionID"][
            "transactionValidStart"]["seconds"] = z[0]
        req["_arg"]["request"]["bodyBytes"]["transactionID"][
            "transactionValidStart"]["nanos"] = z[1]

    def get_node_list(self):
        return self.node_list
    def start(self, context):
        port = context.args[0]

        self.context = context
        self.blockchains = {"group-register": tuple(self.context.doc["test-config"]["hedera-group-topic-id"])}
        self.blockchain_list = ["group-register"]
        self.topic_id_pool = context.doc["test-config"]["hedera-topic-id-pool"]

        # resetting group register
        ii = self.context.path.as_arr()[1]
        sr = self.context.doc["steps"][ii]
        req = copy.deepcopy(sr["messages"]["hedera-message"])
        req["_arg"]["request"]["bodyBytes"] = copy.deepcopy(sr["messages"]["add-group-to-register"]["parts"]["message-body"])
        req["_arg"]["request"]["bodyBytes"]["consensusSubmitMessage"][
            "message"]["alias"] = ""
        req["_arg"]["request"]["bodyBytes"]["consensusSubmitMessage"][
            "message"]["reset_group_register"] = True
        self.set_transaction_id(req)
        self.set_topic_id(req, self.blockchains["group-register"])

        self.context.args = [[req]]
        self.proc_man.do_hedera_calls(self.context)

        sys.stderr.write("starting eternal web demo\n")
        def create_handler(*arg, **kw):
            kw["backend"] = self
            return AdvancedHTTPRequestHandler(*arg, **kw)
        httpd = HTTPServer(('0.0.0.0', port), create_handler)
        httpd.serve_forever()

    def is_current_node_running(self):
        if not self.node_id:
            return False
        return self.node_id in self.node_proc_ids

    def add_group(self, group_id):        
        if not self.topic_id_pool or not self.is_current_node_running():
            return False
        new_topic_id = self.topic_id_pool.pop()

        ii = self.context.path.as_arr()[1]
        sr = self.context.doc["steps"][ii]
        req = copy.deepcopy(sr["messages"]["hedera-message"])
        req["_arg"]["request"]["bodyBytes"] = copy.deepcopy(sr["messages"]["add-group-to-register"]["parts"]["message-body"])

        req["_arg"]["request"]["bodyBytes"]["consensusSubmitMessage"][
            "message"]["alias"] = group_id
        req["_arg"]["request"]["bodyBytes"]["consensusSubmitMessage"][
            "message"]["topic_id"]["shardNum"] = 0
        req["_arg"]["request"]["bodyBytes"]["consensusSubmitMessage"][
            "message"]["topic_id"]["realmNum"] = 0
        req["_arg"]["request"]["bodyBytes"]["consensusSubmitMessage"][
            "message"]["topic_id"]["topicNum"] = new_topic_id

        #print "++++++++++++++++++++"
        #pp = pprint.PrettyPrinter(indent=4)
        #pp.pprint(req)

        self.set_transaction_id(req)
        self.set_topic_id(req, self.blockchains["group-register"])

        self.context.args = [[req]]
        initial = self.get_record_number("group-register")
        self.proc_man.do_hedera_calls(self.context)

        self.wait_for_update(initial, "group-register")

        if not group_id in self.blockchains:
            self.blockchains[group_id] = (0, 0, new_topic_id)
            self.blockchain_list.append(group_id)

            # sending blockchain reset message
            req = copy.deepcopy(sr["messages"]["hedera-message"])
            req["_arg"]["request"]["bodyBytes"] = copy.deepcopy(sr["messages"]["reset-blockchain"]["parts"]["message-body"])
            self.set_transaction_id(req)
            self.set_topic_id(req, self.blockchains[group_id])
            self.context.args = [[req]]
            self.proc_man.do_hedera_calls(self.context)

            # tid = "%d.%d.%d" % self.blockchains[group_id]

            # for i in self.node_list:
            #     tf = "test-stage/%s/%s.%s.bc" % (i, group_id, tid)
            #     os.mkdir(tf)
            #     self.signal_node_to_reload(i)

            return True            

        return False

    def add_node(self, node_id):
        if not node_id:
            return False

        if node_id in self.node_list:
            return False

        cp = self.node_current_endpoint_port
        self.node_current_endpoint_port += 10
        grpc_endpoint = "0.0.0.0:%d" % cp

        self.node_grpc_endpoint[node_id] = grpc_endpoint

        if not self.node_list:
            self.blockchain_id = "group-register"
        self.node_list.append(node_id)
        self.node_id = node_id

        instance_root = "%s/%s" % (self.context.doc[
            "test-config"][
                "test-stage-absolute"], node_id)
        sibling_node_file = "%s/sibling_nodes.txt" % instance_root

        os.mkdir(instance_root)
        #group_register_folder = "%s/group-register.%s.bc" % (instance_root, self.get_tid_str(self.blockchains["group-register"]))
        #os.mkdir(group_register_folder)
        with open(sibling_node_file, "w") as f:
            for i in self.node_grpc_endpoint:
                z = self.node_grpc_endpoint[i]
                if z != grpc_endpoint:
                    f.write("%s\n" % z)

        conf = {
            "worker_count": 10,
            "io_worker_count": 1,
            "data_root_folder": instance_root,
            "hedera_mirror_endpoint": self.context.doc[
                "test-config"]["hedera-mirror-endpoint"],
            "blockchain_append_batch_size": 5,
            "grpc_endpoint": grpc_endpoint,
            "sibling_node_file": sibling_node_file,
            "group_register_topic_id": "0.0.79574",
            "topic_reset_allowed": 1
        }

        self.proc_man.start_gradido_node_with_args(
            instance_root, node_id, conf, [])
        pid = self.proc_man.get_pid(node_id)

        for j in self.node_list:
            if j == node_id:
                continue
            instance_root = "%s/%s" % (self.context.doc[
                "test-config"][
                    "test-stage-absolute"], j),
            sibling_node_file = "%s/sibling_nodes.txt" % instance_root
            with open(sibling_node_file, "w") as f:
                for i in self.node_grpc_endpoint:
                    if i == j:
                        continue
                    z = self.node_grpc_endpoint[i]
                    f.write("%s\n" % z)
            self.signal_node_to_reload(j)

        self.node_proc_ids[node_id] = pid

        # waiting for node to start
        time.sleep(1)
        return True

    def exec_req_sync(self, req, do_wait=True):
        self.context.args = [[req]]
        initial = self.get_record_number(self.blockchain_id)
        self.proc_man.do_hedera_calls(self.context)
        if do_wait:
            self.wait_for_update(initial, self.blockchain_id)

    def attach_blockchain(self):
        if not self.node_id or not self.blockchain_id:
            return

        topic_id = self.get_tid_str(self.blockchains[self.blockchain_id])
        folder = "test-stage/%s/%s.%s.bc" % (self.node_id, 
                                             self.blockchain_id, 
                                             topic_id)
        already_attached = os.path.isdir(folder)
        endpoint = self.node_grpc_endpoint[self.node_id]

        tre = self.proc_man.attach_group(endpoint, 0, 
                                         self.blockchain_id)


        # TODO: fix
        # ii = self.context.path.as_arr()[1]
        # sr = self.context.doc["steps"][ii]
        # req = copy.deepcopy(sr["messages"]["manage-group-message"])
        # req["_arg"]["request"]["action"] = 0
        # req["_arg"]["request"]["group"] = self.blockchain_id

        # self.context.args = [endpoint, [req]]

        # call_obj = self.proc_man.do_grpc_calls(self.context)
        if not already_attached:
            self.wait_for_creation()
        print tre



    def add_user(self, user):
        ii = self.context.path.as_arr()[1]
        sr = self.context.doc["steps"][ii]
        req = copy.deepcopy(sr["messages"]["hedera-message"])
        req["_arg"]["request"]["bodyBytes"] = copy.deepcopy(sr["messages"]["add-user"]["parts"]["message-body"])
        user_pubkey = user[:32]
        user_pubkey += "." * (32 - len(user_pubkey))
        req["_arg"]["request"]["bodyBytes"]["consensusSubmitMessage"][
            "message"]["body_bytes"]["group_member_update"][
                "user_pubkey"] = user_pubkey
        self.set_transaction_id(req)
        self.set_topic_id(req, self.blockchains[self.blockchain_id])
        self.exec_req_sync(req)


    def create_gradidos(self, user, amount):
        ii = self.context.path.as_arr()[1]
        sr = self.context.doc["steps"][ii]
        req = copy.deepcopy(sr["messages"]["hedera-message"])
        req["_arg"]["request"]["bodyBytes"] = copy.deepcopy(sr["messages"]["create-gradidos"]["parts"]["message-body"])
        user_pubkey = user[:32]
        user_pubkey += "." * (32 - len(user_pubkey))
        req["_arg"]["request"]["bodyBytes"]["consensusSubmitMessage"][
            "message"]["body_bytes"]["creation"]["receiver"][
                "pubkey"] = user_pubkey
        req["_arg"]["request"]["bodyBytes"]["consensusSubmitMessage"][
            "message"]["body_bytes"]["creation"]["receiver"][
                "amount"] = amount
        self.set_transaction_id(req)
        self.set_topic_id(req, self.blockchains[self.blockchain_id])
        self.exec_req_sync(req)

    def transfer(self, sender, receiver, amount, receiver_group_id):
        if receiver_group_id == self.blockchain_id:
            self.local_transfer(sender, receiver, amount)
        else:
            self.outbound_transfer(sender, receiver, amount, 
                                   receiver_group_id)

    def local_transfer(self, sender, receiver, amount):
        ii = self.context.path.as_arr()[1]
        sr = self.context.doc["steps"][ii]
        req = copy.deepcopy(sr["messages"]["hedera-message"])
        req["_arg"]["request"]["bodyBytes"] = copy.deepcopy(sr["messages"]["transfer"]["parts"]["message-body"])
        sender_pubkey = sender[:32]
        sender_pubkey += "." * (32 - len(sender_pubkey))
        receiver_pubkey = receiver[:32]
        receiver_pubkey += "." * (32 - len(receiver_pubkey))
        req["_arg"]["request"]["bodyBytes"]["consensusSubmitMessage"][
            "message"]["body_bytes"]["transfer"]["local"]["sender"][
                "pubkey"] = sender_pubkey
        req["_arg"]["request"]["bodyBytes"]["consensusSubmitMessage"][
            "message"]["body_bytes"]["transfer"]["local"]["sender"][
                "amount"] = amount
        req["_arg"]["request"]["bodyBytes"]["consensusSubmitMessage"][
            "message"]["body_bytes"]["transfer"]["local"][
                "receiver"] = receiver_pubkey
        self.set_transaction_id(req)
        self.set_topic_id(req, self.blockchains[self.blockchain_id])
        self.exec_req_sync(req)

    def inbound_transfer(self, sender, receiver, amount,
                         receiver_group_id, paired_transaction_id):
        ii = self.context.path.as_arr()[1]
        sr = self.context.doc["steps"][ii]
        req = copy.deepcopy(sr["messages"]["hedera-message"])
        req["_arg"]["request"]["bodyBytes"] = copy.deepcopy(sr["messages"]["inbound-transfer"]["parts"]["message-body"])
        sender_pubkey = sender[:32]
        sender_pubkey += "." * (32 - len(sender_pubkey))
        receiver_pubkey = receiver[:32]
        receiver_pubkey += "." * (32 - len(receiver_pubkey))
        req["_arg"]["request"]["bodyBytes"]["consensusSubmitMessage"][
            "message"]["body_bytes"]["transfer"]["inbound"]["sender"][
                "pubkey"] = sender_pubkey
        req["_arg"]["request"]["bodyBytes"]["consensusSubmitMessage"][
            "message"]["body_bytes"]["transfer"]["inbound"]["sender"][
                "amount"] = amount
        req["_arg"]["request"]["bodyBytes"]["consensusSubmitMessage"][
            "message"]["body_bytes"]["transfer"]["inbound"][
                "receiver"] = receiver_pubkey
        req["_arg"]["request"]["bodyBytes"]["consensusSubmitMessage"][
            "message"]["body_bytes"]["transfer"]["inbound"][
                "other_group"] = self.blockchain_id
        z = paired_transaction_id
        req["_arg"]["request"]["bodyBytes"]["consensusSubmitMessage"][
            "message"]["body_bytes"]["transfer"]["inbound"][
                "paired_transaction_id"]["seconds"] = z[0]
        req["_arg"]["request"]["bodyBytes"]["consensusSubmitMessage"][
            "message"]["body_bytes"]["transfer"]["inbound"][
                "paired_transaction_id"]["nanos"] = z[1]

        self.set_transaction_id(req)
        self.set_topic_id(req, self.blockchains[receiver_group_id])
        self.exec_req_sync(req)

    def outbound_transfer(self, sender, receiver, amount,
                          receiver_group_id):
        ii = self.context.path.as_arr()[1]
        sr = self.context.doc["steps"][ii]
        req = copy.deepcopy(sr["messages"]["hedera-message"])
        req["_arg"]["request"]["bodyBytes"] = copy.deepcopy(sr["messages"]["outbound-transfer"]["parts"]["message-body"])
        sender_pubkey = sender[:32]
        sender_pubkey += "." * (32 - len(sender_pubkey))
        receiver_pubkey = receiver[:32]
        receiver_pubkey += "." * (32 - len(receiver_pubkey))
        req["_arg"]["request"]["bodyBytes"]["consensusSubmitMessage"][
            "message"]["body_bytes"]["transfer"]["outbound"]["sender"][
                "pubkey"] = sender_pubkey
        req["_arg"]["request"]["bodyBytes"]["consensusSubmitMessage"][
            "message"]["body_bytes"]["transfer"]["outbound"]["sender"][
                "amount"] = amount
        req["_arg"]["request"]["bodyBytes"]["consensusSubmitMessage"][
            "message"]["body_bytes"]["transfer"]["outbound"][
                "receiver"] = receiver_pubkey
        req["_arg"]["request"]["bodyBytes"]["consensusSubmitMessage"][
            "message"]["body_bytes"]["transfer"]["outbound"][
                "other_group"] = receiver_group_id
        z = self.proc_man.get_timestamp_from_epoch(self.context)
        req["_arg"]["request"]["bodyBytes"]["consensusSubmitMessage"][
            "message"]["body_bytes"]["transfer"]["outbound"][
                "paired_transaction_id"]["seconds"] = z[0]
        req["_arg"]["request"]["bodyBytes"]["consensusSubmitMessage"][
            "message"]["body_bytes"]["transfer"]["outbound"][
                "paired_transaction_id"]["nanos"] = z[1]

        self.set_transaction_id(req)
        self.set_topic_id(req, self.blockchains[self.blockchain_id])
        self.exec_req_sync(req, False)
        time.sleep(1)

        self.inbound_transfer(sender, receiver, amount,
                              receiver_group_id, z)
    def rpc_get_users(self):
        if not self.node_id or not self.blockchain_id:
            return
        endpoint = self.node_grpc_endpoint[self.node_id]
        users = self.proc_man.rpc_get_users(endpoint,
                                            self.blockchain_id)
        return users


    def start_curr_node(self):
        if self.node_id and not self.node_id in self.node_proc_ids:
            instance_root = "%s/%s" % (self.context.doc[
                "test-config"][
                    "test-stage-absolute"], self.node_id)
            pid = self.proc_man.restart_gradido_node(instance_root, 
                                                     self.node_id)
            self.node_proc_ids[self.node_id] = pid
        
    def stop_curr_node(self):
        if self.node_id and self.node_id in self.node_proc_ids:
            self.proc_man.stop_gradido_node(self.node_id)
            del self.node_proc_ids[self.node_id]

    def stop(self):
        self.proc_man.cleanup()
        os.system('kill %d' % os.getpid())
    def get_record_number(self, group_id):
        topic_id = self.get_tid_str(self.blockchains[group_id])
        folder = "test-stage/%s/%s.%s.bc" % (self.node_id, 
                                             self.blockchain_id, 
                                             topic_id)
        if not os.path.isdir(folder):
            return 0

        res = subprocess.check_output(["../build/dump_blockchain", "test-stage/%s/%s.%s.bc" % (self.node_id, group_id, topic_id), "-c"], stderr=subprocess.STDOUT)
        return int(res)
    def wait_for_update(self, initial, group_id):
        if not (self.node_id in self.node_proc_ids):
            return # nothing can change, node is down

        topic_id = self.get_tid_str(self.blockchains[group_id])
        folder = "test-stage/%s/%s.%s.bc" % (self.node_id, 
                                             group_id, 
                                             topic_id)
        if not os.path.isdir(folder):
            return 0

        while (True):
            time.sleep(1)
            curr = self.get_record_number(group_id)
            if curr != initial:
                break
    def wait_for_creation(self):
        if not (self.node_id in self.node_proc_ids):
            return # nothing can change, node is down

        group_id = self.blockchain_id
        topic_id = self.get_tid_str(self.blockchains[group_id])
        folder = "test-stage/%s/%s.%s.bc" % (self.node_id, group_id, 
                                             topic_id)
        while (True):
            time.sleep(1)
            if os.path.isdir(folder):
                break
        last = 0
        while (True):
            time.sleep(1)
            curr = self.get_record_number(group_id)
            if curr == last:
                break
            last = curr


    
