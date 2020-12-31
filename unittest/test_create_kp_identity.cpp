#include <Poco/Process.h>
#include <Poco/PipeStream.h>
#include "test_common.h"
#include "utils.h"

#define CREATE_KP_IDENTITY_FOLDER "/tmp/create-kp-identity"

TEST(CreateKpIdentity, smoke) {

    // TODO: fix

    using namespace Poco;

    erase_tmp_folder(CREATE_KP_IDENTITY_FOLDER);
    Poco::File ff(CREATE_KP_IDENTITY_FOLDER);
    ff.createDirectories();

    std::string exe_name = "create_kp_identity";
    Poco::File exe(exe_name);
    exe.copyTo(CREATE_KP_IDENTITY_FOLDER);

    ASSERT_EQ(chdir(CREATE_KP_IDENTITY_FOLDER), 0);
    
    Pipe outPipe;
    Process::Args args;

    ProcessHandle ph(Process::launch(
                     std::string(CREATE_KP_IDENTITY_FOLDER) + "/ " + 
                     exe_name, args, 0, &outPipe, 0));
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
