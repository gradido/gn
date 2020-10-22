import yaml, requests, json, sys, random, time, copy, datetime, traceback, re, os, shutil, subprocess, atexit, signal, math
from pukala import grow, ContextBase, PukalaGrowException, Path, PukalaPathException, FCall
sys.path.append("./hedera/proto_gen")
from hedera.hedera_context import HederaContext
from gradido.gradido_context import GradidoContext
from webapp_demo.webapp_demo_context import WebappDemo, WebappDemoFull

# avoiding unicode mentions in output yaml
def represent_unicode(dumper, data):
    return dumper.represent_scalar(u'tag:yaml.org,2002:str', data)
yaml.add_representer(unicode, represent_unicode)

# disabling stdout buffering
class Unbuffered(object):
   def __init__(self, stream):
       self.stream = stream
   def write(self, data):
       self.stream.write(data)
       self.stream.flush()
   def writelines(self, datas):
       self.stream.writelines(datas)
       self.stream.flush()
   def __getattr__(self, attr):
       return getattr(self.stream, attr)
sys.stdout = Unbuffered(sys.stdout)

# adding possibility to sort yaml dump; TODO: move to pukala library
class ReversedList(list):
    def set_sort_order(self, so):
        self.sort_order = so
    def sort(self, *args, **kwargs):
        def sortf(x):
            xx = x[0]
            return xx in self.sort_order and self.sort_order[xx] or len(self.sort_order) + 1
        res = sorted(self, key=sortf)
        while (len(self)):
            self.pop()
        for i in res:
            self.append(i)
        return res
class ReversedDict(dict):
    def set_sort_order(self, so):
        self.sort_order = so
    def items(self, *args, **kwargs):
        res = ReversedList(dict.items(self, *args, **kwargs))
        res.set_sort_order(self.sort_order)
        return res
yaml.add_representer(ReversedDict, yaml.representer.SafeRepresenter.represent_dict)

# context facets
class SimpleTalk(object):
    def received_hello(self, state):
        return state and state["val"] and "received-hello" in state["val"] and state["val"]["received-hello"] or False
    def contains_hello(self, context):
        return (self.received_hello(context.args[1]) or "hello" in context.args[0])
    def get_answer(self, context):
        if self.received_hello(context.args[0]):
            return "already got your hello"
        else:
            return "hello, alice"
    
class LoginServerContext(object):
    pass

class StepContext(object):
    def __init__(self):
        pass
    def get_input_from(self, context):
        prev_index = len(context.args) > 0 and context.args[0] or -1
        if prev_index >= 0:
            raise Exception("bad previous index %d" % prev_index)
        sub_index = len(context.args) > 1 and context.args[1] or 0
        step_index = context.path.as_arr()[1]
        input_index = step_index + prev_index
        if input_index < 0:
            raise Exception("bad input index %d" % input_index)
        return {
            "src": "/steps/%d/output" % input_index,
            "val": context.doc["steps"][input_index]["output"]
        }
    def get_state_from(self, context):
        step_index = context.path.as_arr()[1]
        instance = context.doc["steps"][step_index]["instance"]
        for i in range(step_index-1, -1, -1):
            if context.doc["steps"][i]["instance"] == instance:
                ii = context.doc["steps"][i]
                src = "/steps/%d/state" % i
                if "new-state" in ii:
                    src = "/steps/%d/new-state" % i
                    val = ii["new-state"]
                else:
                    val = ii["state"]
                if "val" in val:
                    val = val["val"]
                return {
                    "src": src,
                    "val": val
                }
        return None

class HederaServiceContext(object):
    def __init__(self):
        self.current_topic_id = 1
    def append_topic_id(self, context):
        step_index = context.path.as_arr()[1]
        topic_ids = context.doc["steps"][step_index]["state"]["topic-ids"]
        ti = self.current_topic_id
        self.current_topic_id += 1
        return topic_ids + ["0.0.%d" % ti]

    
class TestContext(LoginServerContext, HederaServiceContext, SimpleTalk, HederaContext, StepContext, GradidoContext, ContextBase):
    def __init__(self):
        LoginServerContext.__init__(self)
        HederaServiceContext.__init__(self)
        SimpleTalk.__init__(self)
        HederaContext.__init__(self)
        StepContext.__init__(self)
        GradidoContext.__init__(self)
        WebappDemo.__init__(self)
        ContextBase.__init__(self)
        self.managed_procs = []
        self.webapp_demo = WebappDemo()
        self.webapp_demo_full = WebappDemoFull(self)

    def add_proc(self, proc):
        self.managed_procs.append(proc)
    def remove_proc(self, proc):
        self.managed_procs.remove(proc)
    def cleanup(self):
        for i in self.managed_procs:
            os.killpg(i, signal.SIGKILL)

    def do_sleep(self, context):
        print "sleeping for %d seconds" % context.args[0]
        time.sleep(context.args[0])
        print "woke up"
        return "slept for %d seconds" % context.args[0]

    def get_cmdline_params(self, context):
        res = {}
        for i in sys.argv[2:]:
            z = i.split("=")
            res[z[0]] = z[1]
        return res
    def get_datetime(self, context):
        dt = datetime.datetime.now()
        return dt.strftime("%b %d %Y %H:%M:%S")
    def get_opt(self, context):
        opt_name = context.path.as_arr()[-1]
        if opt_name in context.doc["cmdline-opts"]:
            res = context.doc["cmdline-opts"][opt_name]
            if len(context.args) > 0:
                cf = str(context.args[0])
                if cf == "int":
                    res = int(res)
                elif cf == "float":
                    res = float(res)
                elif cf == "str_arr":
                    res = res.split(",")
                elif cf == "int_arr":
                    res = [int(i) for i in res.split(",")]
                else:
                    raise PukalaPathException("unrecognized get_opt() convert function " + cf, context.path)
            return res
        else:
            return context.doc["default-config"][opt_name]
    def get_seconds_from_epoch(self, context):
        return long(time.time())
    def get_timestamp_from_epoch(self, context):
        z = math.modf(time.time())
        return (long(z[1]), long(z[0] * 1000000000))

    def ensure_test_folder(self, context):
        print "ensuring test folder exists"
        dir_path = os.path.dirname(os.path.realpath(__file__))
        tp = os.path.join(dir_path, context.doc["test-config"]["test-stage"])
        tp = os.path.abspath(tp)
        if not tp.startswith("%s/" % dir_path):
            raise PukalaPathException("test-stage should reside in /test folder, to minimize risk of deleting important data", context.path)
        shutil.rmtree(tp, True)
        os.makedirs(tp)
        print "test folder %s created" % tp
        return tp
    def get_git_commit_hash(self, context):
        return subprocess.check_output(["git", "rev-parse", "--verify", "HEAD"], stderr=subprocess.STDOUT).strip()
        

def prepare_for_dump(doc, regexps, path):
    if isinstance(doc, list):
        res = []
        kk = 0
        for i in doc:
            res.append(prepare_for_dump(i, regexps, "%s%d/" % (path, kk)))
            kk += 1
        return res
    elif isinstance(doc, dict):
        res = {}
        for i in regexps:
            if i[0].match(path):
                res = ReversedDict()
                kk = 1
                so = {}
                for j in i[1]:
                    so[j] = kk
                    kk += 1
                res.set_sort_order(so)
                break
        for i in doc:
            res[i] = prepare_for_dump(doc[i], regexps, "%s%s/" % (path, i))
        return res
    else:
        return doc


def do_output(output_file_name, doc):
    if "_fcall_conf" in doc and "sort" in doc["_fcall_conf"]:
        sort_order = []
        ss = doc["_fcall_conf"]["sort"]
        for i in ss:
            sort_order.append((re.compile(i), ss[i]))
        doc = prepare_for_dump(doc, sort_order, "/")

    if output_file_name and doc:
        with open(output_file_name, "w") as f:
            yaml.dump(doc, f, encoding='utf-8', allow_unicode=True, default_flow_style=False, explicit_start=True)
    elif doc:
        print yaml.dump(doc, encoding='utf-8', allow_unicode=True, default_flow_style=False, explicit_start=True)        

def obtain_file_path_from_config(res, key, default_value):
    zz = default_value
    if (res and "test-config" in res and 
        key in res["test-config"] and 
        isinstance(res["test-config"][key], str)):
        zz = res["test-config"][key]
    return zz

def do_grow():
    if len(sys.argv) < 2:
        raise Exception("provide name of yaml doctree file as command line parameter")
    doctree_file_name = sys.argv[1]

    with open(doctree_file_name, 'r') as f:
        doc = yaml.load(f.read(), Loader=yaml.FullLoader)
    
    tc = TestContext()
    atexit.register(tc.cleanup)

    try:
        print "growing %s" % doctree_file_name
        res = grow(doc, tc)
        test_stage = res["test-config"]["test-stage"]
        output_file_name = os.path.join(test_stage, res["test-config"]["output-file-name"])
        print "writing %s" % output_file_name
        do_output(output_file_name, res)
        output_step_file_name = os.path.join(test_stage, res["test-config"]["output-step-file-name"])
        do_output(output_step_file_name, {"steps": res["steps"]})
    except PukalaGrowException as e:
        test_stage = "/tmp"
        test_stage = obtain_file_path_from_config(e.doc,
                                                  "test-stage", "/tmp")
        output_name = obtain_file_path_from_config(e.doc,
                                                   "output-file-name",
                                                   "ncd-output.yaml")
        output_file_name = os.path.join(test_stage, output_name)
        do_output(output_file_name, e.doc)
        print e.get_formed_desc()
    except Exception as e:
        print traceback.format_exc()


do_grow()
