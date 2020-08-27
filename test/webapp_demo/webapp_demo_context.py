from http.server import HTTPServer, BaseHTTPRequestHandler
import sys, subprocess, time, copy, cgi, os
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

    def show_page(self):
        bchain = subprocess.check_output(["../build/dump_blockchain", "test-stage/gradido-node-0/t_blocks.0.0.113344.bc"], stderr=subprocess.STDOUT)

        zz = json.loads(bchain)
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
        bchain = "<pre>%s</pre>" % json.dumps(zz, indent=4)
        rr = self.create_frame(bchain);
        self.wfile.write(rr)

    def do_GET(self):
        self.send_response(200)
        self.end_headers()
        self.show_page()

    def do_POST(self):
        length = int(self.headers.getheader('content-length'))
        postvars = cgi.parse_qs(self.rfile.read(length), keep_blank_values=1)
        self.send_response(200)
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
        sys.stderr.write("starting eternal web demo\n")
        port = context.args[0]
        def create_handler(*arg, **kw):
            kw["backend"] = self
            return SimpleHTTPRequestHandler(*arg, **kw)
        httpd = HTTPServer(('0.0.0.0', port), create_handler)
        httpd.serve_forever()
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
        self.context.args = [True, [req]]
        self.do_hedera_calls(self.context)
        time.sleep(1)
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
        self.context.args = [True, [req]]
        self.do_hedera_calls(self.context)
        time.sleep(1)
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
        self.context.args = [True, [req]]
        self.do_hedera_calls(self.context)
        time.sleep(1)
    def stop(self):
        self.cleanup()
        os.system('kill %d' % os.getpid())



