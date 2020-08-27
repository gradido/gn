from http.server import HTTPServer, BaseHTTPRequestHandler
import sys, subprocess, time
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
        user_pubkey: <input type="text" id="user">
        <input type="submit" value="add user">
        </form>
        %s
        </body>
</html>
""" % contents

    def show_page(self):
        bchain = subprocess.check_output(["../build/dump_blockchain", "test-stage/gradido-node-0/t_blocks.0.0.113344.bc"], stderr=subprocess.STDOUT)
        bchain = "<pre>%s</pre>" % bchain
        rr = self.create_frame(bchain);
        self.wfile.write(rr)

    def do_GET(self):
        self.send_response(200)
        self.end_headers()
        self.show_page()

    def do_POST(self):
        self.send_response(200)
        self.end_headers()
        self.backend.add_user("buba")
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
        req = sr["messages"]["hedera-message"]
        req["_arg"]["request"]["bodyBytes"] = sr["messages"]["add-user"]["parts"]["message-body"]
        self.context.args = [True, [req]]
        self.do_hedera_calls(self.context)
        time.sleep(1)

