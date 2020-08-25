import os, time, signal, subprocess, json
from pukala import PukalaPathException, Path


class GradidoContext(object):
    def __init__(self):
        self.procs = {}
    def start_gradido_node(self, context):
        inst_base = context.path.as_arr()[:-1]
        ir = Path(inst_base + ["parts", "instance-root"], context.doc)
        ina = Path(inst_base + ["instance"], context.doc)
        cnf = Path(inst_base + ["parts", "conf"], context.doc)
        lgs = Path(inst_base + ["parts", "listening-groups"], context.doc)

        instance_root = ir.get_val(context.doc)
        instance_name = ina.get_val(context.doc)
        conf = cnf.get_val(context.doc)

        if instance_name in self.procs:
            raise PukalaPathException("gradido node instance already initialized", context.path)

        os.mkdir(instance_root)
        conf_file = os.path.join(instance_root, "gradido.conf")
        sibling_file = "/tmp/zzz"
        with open(conf_file, "w") as f:
            for i in conf:
                line = "%s = %s\n" % (i, conf[i])
                f.write(line)
                if i == "sibling_node_file":
                    sibling_file = os.path.join(instance_root, conf[i])
        # TODO: generate
        with open(sibling_file, "w") as f:
            pass
        listening_groups = lgs.get_val(context.doc)
        for i in listening_groups:
            fname = "%(alias)s.%(group_id)d.%(shardNum)d.%(realmNum)d.%(topicNum)d.bc" % i
            fname = os.path.join(instance_root, fname)
            os.mkdir(fname)
            fsnnz_marker = os.path.join(fname, ".is-first-seq-num-non-zero")
            with open(fsnnz_marker, "w") as f:
                f.write("1")

        err_file = os.path.join(instance_root, "err-output.txt")
        cmd = "../build/gradido_node %s 2> %s" % (conf_file, err_file)
        pro = subprocess.Popen(cmd, stdout=subprocess.PIPE, 
                               shell=True, preexec_fn=os.setsid) 
        self.add_proc(pro.pid)
        self.procs[instance_name] = (instance_root, pro, err_file)
        return "launched"
    def finish_gradido_node(self, context):
        inst_base = context.path.as_arr()[:-1]
        ina = Path(inst_base + ["instance"], context.doc)
        instance_name = ina.get_val(context.doc)

        if not instance_name in self.procs:
            raise PukalaPathException("gradido node instance not initialized", context.path)

        (instance_root, pro, err_file) = self.procs[instance_name]
        os.killpg(os.getpgid(pro.pid), signal.SIGTERM) 
        self.remove_proc(pro.pid)
        time.sleep(1)
        with open(err_file) as f:
            lines = f.read().split("\n")

        bchains = {}
        for i in os.listdir(instance_root):
            pp = os.path.join(instance_root, i)
            if os.path.isdir(pp):
                bchain = subprocess.check_output(["../build/dump_blockchain", pp], stderr=subprocess.STDOUT)
                if bchain:
                    print "hahah-------------", bchain
                    bchains[i] = json.loads(bchain)
                else:
                    bchains[i] = None

        del self.procs[instance_name]
        return {
            "stderr": lines,
            "blockchains": bchains
        }
    def get_instance_root(self, context):
        return "%s/gradido-node-0" % context.doc["test-config"]["test-stage-absolute"]
    def get_sibling_node_file(self, context):
        inst_base = context.path.as_arr()[:-2]
        ina = Path(inst_base + ["instance-root"], context.doc)
        return "%s/sibling_nodes.txt" % ina.get_val(context.doc)


        
        
