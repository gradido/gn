#include <Poco/Process.h>
#include <Poco/PipeStream.h>
#include <Poco/Path.h>
#include "test_common.h"
#include "utils.h"

#define CREATE_KP_IDENTITY_FOLDER "/tmp/create-kp-identity"

TEST(CreateKpIdentity, smoke) {
    using namespace Poco;

    std::string build_dir = Poco::Path::current();
    std::string exe_name = "create_kp_identity";

    erase_tmp_folder(CREATE_KP_IDENTITY_FOLDER);
    Poco::File ff(CREATE_KP_IDENTITY_FOLDER);
    ff.createDirectories();

    ASSERT_EQ(chdir(CREATE_KP_IDENTITY_FOLDER), 0);
    
    Pipe outPipe;
    Process::Args args;

    ProcessHandle ph(Process::launch(
                     build_dir + "/" + exe_name, args, 0, &outPipe, 0));
    PipeInputStream istr(outPipe);
    int rc = ph.wait();

    sleep(1);

    Poco::File priv(KP_IDENTITY_PRIV_NAME);
    ASSERT_EQ(priv.isFile(), true);
    ASSERT_EQ(priv.getSize(), KEY_LENGTH_HEX);

    Poco::File pub(KP_IDENTITY_PUB_NAME);
    ASSERT_EQ(pub.isFile(), true);
    ASSERT_EQ(pub.getSize(), KEY_LENGTH_HEX);
}
