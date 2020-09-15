#include "test_common.h"
#include "blockchain_gradido_translate.h"
#include "utils.h"
#include <fstream>


#define EMPTY_KEY "12345678901234567890123456789012"
#define EMPTY_KEY_B "1234567890123456789012345678901b"
#define EMPTY_KEY_C "1234567890123456789012345678901c"
#define EMPTY_KEY_D "1234567890123456789012345678901d"

#define EMPTY_HEDERA_HASH "123456789012345678901234567890123456789012345678"

void prepare_ctr_template(ConsensusTopicResponse& ctr0,
                          std::function<void(grpr::TransactionBody&)> tb_cb,
                          std::function<void(grpr::SignatureMap&)> sig_cb) {
    ctr0.set_sequencenumber(1);
    ctr0.set_runninghashversion(2);
    ctr0.set_runninghash(EMPTY_HEDERA_HASH);

    proto::Timestamp* ts = ctr0.mutable_consensustimestamp();
    ts->set_seconds(13);
    ts->set_nanos(21);

    grpr::GradidoTransaction gt;
    grpr::SignatureMap* sig_map = gt.mutable_sig_map();
    grpr::TransactionBody tb;
    tb.set_version_number(1);

    sig_cb(*sig_map);
    tb_cb(tb);

    std::string tb_bytes;
    tb.SerializeToString(&tb_bytes);
    gt.set_body_bytes(tb_bytes);
    std::string gt_bytes;
    gt.SerializeToString(&gt_bytes);
    ctr0.set_message(gt_bytes);
}

void check_translate_eq(
     std::function<void(grpr::TransactionBody&)> tb_cb,
     std::function<void(grpr::SignatureMap&)> sig_cb) {

    ConsensusTopicResponse ctr0;
    prepare_ctr_template(ctr0, tb_cb, sig_cb);

    MultipartTransaction mt0;
    TransactionUtils::translate_from_ctr(ctr0, mt0);

    //dump_transaction_in_json(mt0.rec[0], std::cerr);
    
    ConsensusTopicResponse ctr1;
    TransactionUtils::translate_to_ctr(mt0, ctr1);
    std::string b0;
    std::string b1;
    ctr0.SerializeToString(&b0);
    ctr1.SerializeToString(&b1);

    /*
    std::ofstream f0("/tmp/dump-b0.bin", std::ios::binary | std::ios::out);
    f0.write(b0.c_str(), b0.size());
    f0.close();
    std::ofstream f1("/tmp/dump-b1.bin", std::ios::binary | std::ios::out);
    f1.write(b1.c_str(), b1.size());
    f1.close();
    */
    ASSERT_EQ(b0.compare(b1), 0);
}

void check_translate_eq(std::function<void(grpr::TransactionBody&)> 
                        tb_cb) {
    check_translate_eq(tb_cb, [](grpr::SignatureMap& sig_map) {
            grpr::SignaturePair* sig_pair = sig_map.add_sig_pair();
            sig_pair->set_pub_key(EMPTY_KEY);
            sig_pair->set_ed25519(EMPTY_KEY_B);
        });
}


TEST(Translate, translate_eq_add_friend) 
{
    check_translate_eq([](grpr::TransactionBody& tb) {
            auto gfu = tb.mutable_group_friends_update();
            gfu->set_group("group-A");
            gfu->set_action(grpr::GroupFriendsUpdate::ADD_FRIEND);
        });
}

TEST(Translate, translate_eq_remove_friend) 
{
    check_translate_eq([](grpr::TransactionBody& tb) {
            auto gfu = tb.mutable_group_friends_update();
            gfu->set_group("group-A");
            gfu->set_action(grpr::GroupFriendsUpdate::REMOVE_FRIEND);
        });
}

TEST(Translate, translate_eq_creation) 
{
    check_translate_eq([](grpr::TransactionBody& tb) {
            auto cr = tb.mutable_creation();
            auto receiver = cr->mutable_receiver();
            receiver->set_amount(79);
            receiver->set_pubkey(EMPTY_KEY_C);
        });
}

TEST(Translate, translate_eq_add_user) 
{
    check_translate_eq([](grpr::TransactionBody& tb) {
            auto gmu = tb.mutable_group_member_update();
            gmu->set_member_update_type(grpr::GroupMemberUpdate::ADD_USER);
            gmu->set_user_pubkey(EMPTY_KEY_C);
        });
}

TEST(Translate, translate_eq_move_user_inbound) 
{
    check_translate_eq([](grpr::TransactionBody& tb) {
            auto gmu = tb.mutable_group_member_update();
            gmu->set_member_update_type(grpr::GroupMemberUpdate::MOVE_USER_INBOUND);
            gmu->set_user_pubkey(EMPTY_KEY_C);
            gmu->set_target_group("group-B");
            proto::Timestamp* ts = gmu->mutable_paired_transaction_id();
            ts->set_seconds(17);
            ts->set_nanos(61);
        });
}

TEST(Translate, translate_eq_move_user_outbound) 
{
    check_translate_eq([](grpr::TransactionBody& tb) {
            auto gmu = tb.mutable_group_member_update();
            gmu->set_member_update_type(grpr::GroupMemberUpdate::MOVE_USER_OUTBOUND);
            gmu->set_user_pubkey(EMPTY_KEY_C);
            gmu->set_target_group("group-B");
            proto::Timestamp* ts = gmu->mutable_paired_transaction_id();
            ts->set_seconds(17);
            ts->set_nanos(61);
        });
}

TEST(Translate, translate_eq_local_transfer) 
{
    check_translate_eq([](grpr::TransactionBody& tb) {
            auto tr = tb.mutable_transfer();
            auto tt = tr->mutable_local();
            auto sender = tt->mutable_sender();
            sender->set_pubkey(EMPTY_KEY_C);
            sender->set_amount(79);
            tt->set_receiver(EMPTY_KEY_D);
        });
}

TEST(Translate, translate_eq_inbound_transfer) 
{
    check_translate_eq([](grpr::TransactionBody& tb) {
            auto tr = tb.mutable_transfer();
            auto tt = tr->mutable_inbound();
            auto sender = tt->mutable_sender();
            sender->set_pubkey(EMPTY_KEY_C);
            sender->set_amount(79);
            tt->set_receiver(EMPTY_KEY_D);
            tt->set_other_group("group-B");
            proto::Timestamp* ts = tt->mutable_paired_transaction_id();
            ts->set_seconds(17);
            ts->set_nanos(61);
        });
}

TEST(Translate, translate_eq_outbound_transfer) 
{
    check_translate_eq([](grpr::TransactionBody& tb) {
            auto tr = tb.mutable_transfer();
            auto tt = tr->mutable_outbound();
            auto sender = tt->mutable_sender();
            sender->set_pubkey(EMPTY_KEY_C);
            sender->set_amount(79);
            tt->set_receiver(EMPTY_KEY_D);
            tt->set_other_group("group-B");
            proto::Timestamp* ts = tt->mutable_paired_transaction_id();
            ts->set_seconds(17);
            ts->set_nanos(61);
        });
}

TEST(Translate, translate_eq_multipart_memo) 
{
    check_translate_eq([](grpr::TransactionBody& tb) {
            auto gmu = tb.mutable_group_member_update();
            gmu->set_member_update_type(grpr::GroupMemberUpdate::ADD_USER);
            gmu->set_user_pubkey(EMPTY_KEY_C);

            char memo[MEMO_FULL_SIZE];
            int cnt = 1;
            for (int i = 0; i < MEMO_FULL_SIZE - 1; i++) {
                memo[i] = '0' + (char)cnt;
                cnt++;
                cnt = cnt % 10;
            }
            memo[MEMO_FULL_SIZE-1] = 0;
            tb.set_memo(std::string(memo));
        });
}


TEST(Translate, translate_eq_many_signatures) 
{
    check_translate_eq([](grpr::TransactionBody& tb) {
            auto gmu = tb.mutable_group_member_update();
            gmu->set_member_update_type(grpr::GroupMemberUpdate::ADD_USER);
            gmu->set_user_pubkey(EMPTY_KEY_C);
        }, [](grpr::SignatureMap& sig_map) {
            for (int i = 0; i < SIGNATURES_PER_RECORD * (MAX_RECORD_PARTS - NON_SIGNATURE_PARTS) + 1; i++) {
                grpr::SignaturePair* sig_pair = sig_map.add_sig_pair();
                sig_pair->set_pub_key(EMPTY_KEY);
                sig_pair->set_ed25519(EMPTY_KEY_B);
            }
        });
}

TEST(Translate, translate_eq_multipart_memo_and_many_signatures) 
{
    check_translate_eq([](grpr::TransactionBody& tb) {
            auto gmu = tb.mutable_group_member_update();
            gmu->set_member_update_type(grpr::GroupMemberUpdate::ADD_USER);
            gmu->set_user_pubkey(EMPTY_KEY_C);

            char memo[MEMO_FULL_SIZE];
            int cnt = 1;
            for (int i = 0; i < MEMO_FULL_SIZE - 1; i++) {
                memo[i] = '0' + (char)cnt;
                cnt++;
                cnt = cnt % 10;
            }
            memo[MEMO_FULL_SIZE-1] = 0;
            tb.set_memo(std::string(memo));
        }, [](grpr::SignatureMap& sig_map) {
            for (int i = 0; i < SIGNATURES_PER_RECORD * (MAX_RECORD_PARTS - NON_SIGNATURE_PARTS) + 1; i++) {
                grpr::SignaturePair* sig_pair = sig_map.add_sig_pair();
                sig_pair->set_pub_key(EMPTY_KEY);
                sig_pair->set_ed25519(EMPTY_KEY_B);
            }
        });
}



