#include <Poco/Process.h>
#include <Poco/PipeStream.h>
#include <Poco/Path.h>
#include <fstream>
#include "test_common.h"
#include "utils.h"
#include "worker_pool.h"
#include "facades_impl.h"
#include "blockchain_base.h"


#define NODES_TEST_FOLDER "/tmp/nodes-test-folder"

using namespace Poco;
using namespace gradido;

static EmptyFacade empty_facade;
static WorkerPool wp(&empty_facade, "test"); // for err writing
static std::string build_dir;
void init_build_dir() {
    if (build_dir.length() == 0) {
        build_dir = Poco::Path::current();
        build_dir = build_dir.substr(0, build_dir.length() - 1);
        wp.init(10); // should be at least as much as concurrent nodes
    }
}
void set_instance(std::string instance_path) {
    init_build_dir();
    if (chdir(instance_path.c_str()) != 0)
        PRECISE_THROW(std::string("couldn't chdir to ") + 
                                 instance_path);
}

std::string prepare_instance(std::string relative_folder) {
    std::string z = std::string(NODES_TEST_FOLDER) + "/" + 
        relative_folder;
    Poco::File ff(z);
    ff.createDirectories();
    return z;
}

void write_gradido_conf(std::map<std::string, std::string> map,
                        std::string instance_path) {
    set_instance(instance_path);
    std::string conf_file = instance_path + "/gradido.conf";
    std::ofstream f0(conf_file, std::ios::out);
    for (auto i : map) {
        std::string s = i.first + " = " + i.second + "\n";
        f0.write(s.c_str(), s.length());
    }
    f0.close();
}

void create_launch_token(std::string launcher_instance,
                         std::string instance_path) {
    set_instance(launcher_instance);

    std::string launcher_pub_key_file = launcher_instance + 
        "/identity-pub.conf";

    std::ifstream in(launcher_pub_key_file);
    std::string str;
    std::getline(in, str);
    if (str.size() != KEY_LENGTH_HEX)
        PRECISE_THROW("cannot get launcher pub key");
    in.close();

    set_instance(instance_path);
    std::string conf_file = instance_path + "/launch-token.conf";
    std::ofstream f0(conf_file, std::ios::out);
    std::string s = "admin_key = " + str + "\n";
    f0.write(s.c_str(), s.length());
    f0.close();
}

class TestProcess {
private:
    Pipe outPipe;
    Pipe errPipe;
    ProcessHandle ph;
    PipeInputStream istr;
    PipeInputStream estr;

    static std::vector<TestProcess*> processes;

    class ErrWriterTask : public ITask {
    private:
        PipeInputStream* estr;
        std::string out;
    public:
        ErrWriterTask(PipeInputStream* estr, 
                      std::string out) : estr(estr), out(out) {}
        virtual void run() {
            std::ofstream f0(out, std::ios::out | std::ios::app);
            while (1) {
                std::string line;
                std::getline(*estr, line);
                if (line.length() == 0)
                    break;
                f0 << line << std::endl;
            }
            f0.close();
        }
    };

public:
    TestProcess(std::string exe_name,
                Process::Args args,
                std::string err_file) :
        ph(Process::launch(exe_name, args, 0, &outPipe, &errPipe)),
        istr(outPipe), estr(errPipe) {
        processes.push_back(this);
        wp.push(new ErrWriterTask(&estr, err_file));
    }

    void wait() {
        ph.wait();
    }

    void kill() {
        if (use_valgring)
            Poco::Process::requestTermination(ph.id());
        else
            Poco::Process::kill(ph);
    }

    static void clear_processes() {
        for (auto i : processes) {

            try {
                i->kill();
                i->wait();
            } catch (...) {
                // nothing here
            }

            delete i;
        }
        processes.clear();
    }
};
std::vector<TestProcess*> TestProcess::processes;

class GlobalNodeOwner {
public:
    GlobalNodeOwner() {
        //LOG("taking global node ownership");
        erase_tmp_folder(NODES_TEST_FOLDER);
        init_build_dir();
        while (wp.get_busy_worker_count() > 0) {
            LOG("waiting for " + std::to_string(
                                 wp.get_busy_worker_count()) +
                " workers to prepare");
            sleep(1);
        }
    }
    ~GlobalNodeOwner() {
        //LOG("releasing global node ownership");
        sleep(1); // waiting for writes to finish
        TestProcess::clear_processes();
        sleep(1); // giving time for potential gtest exit
    }
};

std::string get_err_file(std::string exe_name0, 
                         std::string instance_path) {
    return instance_path + "/" + exe_name0 + ".err-output.txt";
}

TestProcess* start_process(std::string exe_name0, 
                           Process::Args args,
                           std::string instance_path) {
    set_instance(instance_path);

    std::string exe_name = build_dir + "/" + exe_name0;
    std::string err_file = get_err_file(exe_name0, instance_path);

    if (use_valgring) {
        Process::Args args2 = args;
        args2.insert(args2.begin(), exe_name);
        return new TestProcess("valgrind", args2, err_file);
    } else {
        return new TestProcess(exe_name, args, err_file);
    }
}

TestProcess* start_process(std::string exe_name0, 
                           std::string instance_path) {
    Process::Args args;
    return start_process(exe_name0, args, instance_path);
}

bool errlog_ends_with(std::string exe_name0, 
                      std::string instance_path,
                      std::string suffix) {
    std::string err_file = get_err_file(exe_name0, instance_path);
    std::ifstream in(err_file);
    std::string str;
    std::string str2;
    while (std::getline(in, str)) {
        if (use_valgring) {
            if (str.length() == 0 || 
                (str.length() > 1 && (str[0] == '=' && str[1] == '='))) {
                // ignoring
            } else
                str2 = str;
        } else
            str2 = str;
    }

    return ends_with(str2, suffix);
}

void add_kp(std::string launcher_instance) {
    start_process("create_kp_identity",
                  launcher_instance)->wait();
    if (!errlog_ends_with(
          "create_kp_identity",
          launcher_instance,
          "keys created in identity-priv.conf and identity-priv.conf"))
        PRECISE_THROW("create_kp_identity failed");
}

void launch_launcher(std::string own_endpoint,
                     std::string target_endpoint,
                     std::string launcher_instance) {
    Process::Args args;
    args.push_back(own_endpoint);
    args.push_back(target_endpoint);
    start_process("node_launcher", args, 
                  launcher_instance)->wait();
    if (!errlog_ends_with(
                  "node_launcher",
                  launcher_instance,
                  "launcher finished successfully"))
        PRECISE_THROW("launcher failed");
}

std::string prepare_admin_instance(std::string name) {
    std::string admin_instance = prepare_instance(name);
    add_kp(admin_instance);

    set_instance(admin_instance);
    Process::Args args;
    args.push_back(name);
    args.push_back(name + "@gmail.com");
    start_process("create_admin_enquiry", args, 
                  admin_instance)->wait();
    
    if (!errlog_ends_with(
                  "create_admin_enquiry",
                  admin_instance,
                  "enquiry created"))
        PRECISE_THROW("enquiry failed");
    return admin_instance;
}

void init_sb(std::string instance, std::string name) {
    set_instance(instance);
    Process::Args args;
    args.push_back(name);
    start_process("create_sb", args, instance)->wait();
    
    if (!errlog_ends_with(
                  "create_sb",
                  instance,
                  "sb created"))
        PRECISE_THROW("subcluster blockchain creation failed");
}

void put_sb_on_primary_ordering(std::string launcher_instance, 
                                std::string ordering_instance) {
    std::string launcher_pub_key_file = launcher_instance + 
        "/identity-pub.conf";
    std::ifstream in(launcher_pub_key_file);
    std::string pub_key;
    std::getline(in, pub_key);
    if (pub_key.size() != KEY_LENGTH_HEX)
        PRECISE_THROW("cannot get launcher pub key");
    in.close();

    std::string sb_name = pub_key + ".bc";
    Poco::File from(launcher_instance + "/" + sb_name);
    from.copyTo(ordering_instance + "/" + sb_name);
}

void add_admin(std::string launcher_instance, 
               std::string admin_instance) {
    Poco::File from(admin_instance + "/admin-enquiry.conf");
    from.copyTo(launcher_instance + "/admin-enquiry.conf");
    set_instance(launcher_instance);
    Process::Args args;
    start_process("add_admin_to_sb", args, launcher_instance)->wait();
    
    if (!errlog_ends_with(
                  "add_admin_to_sb",
                  launcher_instance,
                  "admin added"))
        PRECISE_THROW("adding admin to subcluster failed");
}

/*
TEST(GradidoNodes, test_launch_logger) {
    std::string logger_instance;

    {
        GlobalNodeOwner gno;
        std::string launcher_instance = 
            prepare_instance("node_launcher");

        logger_instance = prepare_instance("logger");

        std::string logger_endpoint = "0.0.0.0:13711";
        std::string sb_ordering_node_endpoint = "0.0.0.0:13710";
        std::string sb_pub_key = "283712F47FB54CF81E51D592857E5A91";
        std::string sb_ordering_node_pub_key = 
            "283712F47FB54CF81E51D592857E5A91";
        std::string launcher_endpoint = "0.0.0.0:13701";

        
        add_kp(launcher_instance);

        {
            std::map<std::string, std::string> conf;
            conf.insert({"data_root_folder", "."});
            conf.insert({"grpc_endpoint", logger_endpoint});
            conf.insert({"blockchain_append_batch_size", "10"});
            conf.insert({"general_batch_size", "1000"});
            conf.insert({"is_sb_host", "0"});
            conf.insert({"sb_ordering_node_endpoint", 
                        sb_ordering_node_endpoint});
            conf.insert({"sb_pub_key", sb_pub_key});
            conf.insert({"sb_ordering_node_pub_key",
                        sb_ordering_node_pub_key});
            write_gradido_conf(conf, logger_instance);
            create_launch_token(launcher_instance, logger_instance);

            start_process("logger", logger_instance);
        }
        launch_launcher(launcher_endpoint, logger_endpoint,
                        launcher_instance);
    }

    ASSERT_EQ(errlog_ends_with(
              "logger",
              logger_instance,
              "submitting own entry to subcluster blockchain"), 
              true);
}
*/

TEST(GradidoNodes, test_launch_initial_ordering) {
    std::string ordering_instance;

    {
        GlobalNodeOwner gno;

        std::string admin0_instance = prepare_admin_instance("admin0");
        std::string admin1_instance = prepare_admin_instance("admin1");
        std::string admin2_instance = prepare_admin_instance("admin2");

        std::string launcher_instance = 
            prepare_instance("node_launcher");
        add_kp(launcher_instance);
        init_sb(launcher_instance, "subcluster0");

        add_admin(launcher_instance, admin0_instance);
        add_admin(launcher_instance, admin1_instance);
        add_admin(launcher_instance, admin2_instance);

        ordering_instance = prepare_instance("ordering");

        std::string ordering_endpoint = "0.0.0.0:13710";
        std::string sb_ordering_node_endpoint = "0.0.0.0:13710";
        std::string sb_pub_key = "283712F47FB54CF81E51D592857E5A91";
        std::string sb_ordering_node_pub_key = 
            "283712F47FB54CF81E51D592857E5A91";
        std::string launcher_endpoint = "0.0.0.0:13701";

        {
            std::map<std::string, std::string> conf;
            conf.insert({"data_root_folder", "."});
            conf.insert({"grpc_endpoint", ordering_endpoint});
            conf.insert({"blockchain_append_batch_size", "10"});
            conf.insert({"general_batch_size", "1000"});
            conf.insert({"is_sb_host", "1"});
            conf.insert({"sb_pub_key", sb_pub_key});
            write_gradido_conf(conf, ordering_instance);
            create_launch_token(launcher_instance, ordering_instance);
            put_sb_on_primary_ordering(launcher_instance, 
                                       ordering_instance);
            start_process("ordering", ordering_instance);
        }

        launch_launcher(launcher_endpoint, ordering_endpoint,
                        launcher_instance);
    }

}

