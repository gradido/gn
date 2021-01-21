-module(gradido).
-export([
         sig_init/0, sig_create_pk_pair/0, sig_sign/2, sig_verify/2,
         bc_create/0, bc_append/2, bc_verify_checksum/1, bc_extract_checksums/1,

         nd_start_node/1, nd_ordering_node/1,

         test/0
        ]).

-define(BLOCK_SIZE, 10).
-define(CA_MASTER, 123).
-define(CA_MASTER_PUB, ?CA_MASTER * 1000).

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% signatures

sig_init() ->
    % better to have static seed, to be able to repeat tests
    _ = rand:seed(exs1024, 123),
    ok.


% signatures will be dummy, to have them readable; returns 
% {private, public}
sig_create_pk_pair() ->
    Z = rand:uniform(999),
    {key_pair, Z, Z * 1000}.

sig_sign(Data, {key_pair, PrivateKey, _}) when is_integer(PrivateKey) ->
    {signature, PrivateKey * 1000, erlang:phash2({Data, PrivateKey})};
sig_sign(Data, PrivateKey) when is_integer(PrivateKey) ->
    {signature, PrivateKey * 1000, erlang:phash2({Data, PrivateKey})};
sig_sign(_, _) ->
    false.


sig_verify(_, []) ->
    false;
sig_verify(Data, Signature={signature, PublicKey, _}) ->
    Z = sig_sign(Data, round(PublicKey / 1000)),
    Z == Signature;
sig_verify(Data, [H]) ->
    sig_verify(Data, H);
sig_verify(Data, [H|T]) ->
    case sig_verify(Data, H) of
        false ->
            false;
        _ ->
            sig_verify(Data, T)
    end;
sig_verify(_, _) ->
    false.


sig_derive({key_pair, PrivateKey, _}) ->
    P = PrivateKey + 1,
    {key_pair, P, P * 1000};
sig_derive(PrivateKey) ->
    P = PrivateKey + 1,
    {key_pair, P, P * 1000}.

sig_is_derived(ParentPubKey, ChildPubKey) ->
    ParentPubKey + 1000 == ChildPubKey.

sig_verify_chain([]) ->
    false;
sig_verify_chain([Last]) ->
    sig_is_derived(?CA_MASTER_PUB, Last);
sig_verify_chain([H,H1|T]) ->
    case sig_is_derived(H1, H) of
        false ->
            false;
        true ->
            sig_verify_chain([H1|T])
    end.

sig_sign_packet(Data, PrivateKey) ->
    {Data, sig_sign(Data, PrivateKey)}.
    

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% misc
msc_timestamp() ->
    erlang:timestamp().


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% endpoints
ep_send(_Endpoint={endpoint, Pid}, Data, SenderPrivKey) ->
    Packet = sig_sign_packet({Data, msc_timestamp()}, SenderPrivKey),
    Pid ! Packet,
    ok.

ep_spawn(Module, Func, Data) ->
    {endpoint, spawn(Module, Func, [Data])}.

ep_create() ->
    {endpoint, self()}.

ep_receive() ->
    receive
        Any ->
            erlang:display({received, Any}),
            Any
    end.
    
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% blockchains

bc_zero_checksum() ->
    {checksum, 0}.

bc_create() ->
    {blockchain, [], 0, 0}.

bc_is_blockchain({blockchain, Blocks, BC, LBS})
  when is_list(Blocks), is_integer(BC), is_integer(LBS) ->
    true;
bc_is_blockchain(_) ->
    false.
        

bc_checksum(Rec, {checksum, PrevChecksum}) ->
    {checksum, erlang:phash2({Rec, PrevChecksum})};
bc_checksum(_, _) ->
    not_a_checksum.

bc_append({blockchain, [], 0, 0}, Rec) ->
    {blockchain, [[bc_checksum(Rec, bc_zero_checksum()), Rec]], 1, 2};
bc_append({blockchain, Blocks=[[PrevChecksum={checksum, _}|_]|_], 
           BlockCount, ?BLOCK_SIZE}, Rec) ->
    {blockchain, [[bc_checksum(Rec, PrevChecksum), Rec]|Blocks], 
     BlockCount + 1, 2};
bc_append({blockchain, [[PrevChecksum={checksum, _}|LastBlockRest]|
                        RestBlocks], 
           BlockCount, LastBlockSize}, Rec) ->
    {blockchain, [[bc_checksum(Rec, PrevChecksum)|[Rec|LastBlockRest]]|
                  RestBlocks], 
     BlockCount, LastBlockSize + 1}.

bc_do_extract_checksums({blockchain, [], A, B}, Acc) ->
    {checksums, Acc, (A - 1) * ?BLOCK_SIZE + B};
bc_do_extract_checksums({blockchain, [[H|_]|T], A, B}, Acc) ->
    bc_do_extract_checksums({blockchain, T, A, B}, [H|Acc]).

bc_extract_checksums(B) ->
    bc_do_extract_checksums(B, []).

bc_extract_records({blockchain, [], 0, 0}, Acc) ->
    Acc;
bc_extract_records({blockchain, [], 0, _}, _) ->
    bad_last_block_size;
bc_extract_records({blockchain, [], _, 0}, _) ->
    bad_block_count;
bc_extract_records({blockchain, [], _, _}, _) ->
    bad_block_count_and_last_block_size;
bc_extract_records({blockchain, _, BlockCount, _}, _) when BlockCount < 0 ->
    bad_block_count2;
bc_extract_records({blockchain, _, _, LastBlockSize}, _) when LastBlockSize < 0 ->
    bad_last_block_size2;
bc_extract_records({blockchain, _, BlockCount, LastBlockSize}, _) when LastBlockSize < 0, BlockCount < 0 ->
    bad_block_count_and_last_block_size2;
bc_extract_records({blockchain, [[]], BlockCount, LastBlockSize},
                      Acc) -> 
    bc_extract_records({blockchain, [], BlockCount - 1, 
                           LastBlockSize}, Acc);
bc_extract_records({blockchain, [[]|_], _, 
                       LastBlockSize}, _) when LastBlockSize > 0 -> 
    bad_last_block_size3;
bc_extract_records({blockchain, [[]|BT], BlockCount, 
                       _LastBlockSize}, Acc) -> 
    bc_extract_records({blockchain, BT, BlockCount - 1, ?BLOCK_SIZE}, 
                         Acc);
bc_extract_records({blockchain, [[CBH|CBT]|BT], BlockCount, 
                       ?BLOCK_SIZE}, 
                      Acc) -> 
    case CBH of
        {checksum, _} ->
            bc_extract_records({blockchain, [CBT|BT], BlockCount, 
                                   ?BLOCK_SIZE - 1}, Acc);
        _ ->
            checksum_expected
    end;
bc_extract_records({blockchain, [[CBH|CBT]|BT], BlockCount, 
                       LastBlockSize}, 
                      Acc) -> 
    bc_extract_records({blockchain, [CBT|BT], BlockCount, 
                           LastBlockSize - 1}, [CBH|Acc]).

bc_do_verify_records([], C, C) ->
    checksums_match;
bc_do_verify_records([], _, _) ->
    checksums_dont_match;
bc_do_verify_records([H|T], C0, C1) ->
    C = bc_checksum(H, C0),
    bc_do_verify_records(T, C, C1).

bc_verify_checksum({blockchain, [], 0, 0}) ->
    ok;
bc_verify_checksum({blockchain, [[]], 0, 0}) ->
    ok;
bc_verify_checksum(
  {blockchain, Blocks=[[LastChecksum={checksum, _}|LB]|B],
   BlockCount, LastBlockSize})
  when is_integer(BlockCount), is_integer(LastBlockSize), 
       is_list(Blocks) ->
    R = bc_extract_records({blockchain, [LB|B], BlockCount, 
                            LastBlockSize - 1}, []),
    case is_list(R) of
        true ->
            bc_do_verify_records(R, bc_zero_checksum(), LastChecksum);
        _ ->
            R
    end;
bc_verify_checksum(
  {blockchain, Blocks, BlockCount, LastBlockSize})
  when is_integer(BlockCount), is_integer(LastBlockSize), 
       is_list(Blocks) ->
    last_record_not_checksum_or_bad_block_structure;
bc_verify_checksum(_) ->
    not_a_blockchain.


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% subcluster blockchain

% blockchain structure
% - header
%   - subclaster name
%   - initial key type (ca_signed, self_signed)
%   - list of public keys, representing CA chain (if type is ca_signed,
%     chain is verified with CA_MASTER_PUB, which is hardcoded)
%     - first public key is considered initial key
% - one or more initial admins (preferrably, 3)
%   - email
%   - name
%   - public key
%   - signature with initial key
%     - all fields concatenated for signing in their respective order
% - ordering node (it is associated with subclaster blockchain in 1:1
%   relationship)
%   - type (ordering node)
%   - endpoint
%   - public key
%   - signature with any of admins
%
% from this point all other nodes follow in arbitrary order; admins can
% be added or removed by majority of admins signing the record (majority
% of 2 is 2); most of the nodes are described by their type, endpoint,
% public key
%
% data in subclaster blockchain is used for gradido blockchains
% 
% gradido blockchain startup sequence
% - admin sends transaction, containing blockchain's public key and
%   ordering node public key to sb
%   - ordering node starts listening
% - admin sends transaction, containing blockchain's public key and
%   login node public key to sb
%   - login node starts listening
% - admin sends transaction, containing blockchain's public key and
%   gradido node to sb
%   - gradido node connects to ordering node and starts listening
% - admin sends header and public keys of various involved nodes to
%   ordering service, so they are included in gradido blockchain
% - then initialization done transaction is sent to gradido blockchain
% - at this point gradido blockchain is ready for transactions




-record(subl, {
               bc=bc_create(),
               admins=dict:new(),
               o_nodes=[],
               i_key={}
            }).

% as a rule of thumb, following functions won't check index integrity

sb_create(Type=self_signed, InitKeyChain=[H|_], Name) ->
    B0 = #subl{},
    B0#subl{bc=bc_append(B0#subl.bc, {subclaster_blockchain, Name, 
                                      {Type, InitKeyChain}}),
            i_key=H};
sb_create(Type=ca_signed, InitKeyChain=[H|_], Name) ->
    case sig_verify_chain(InitKeyChain) of
        true ->
            B0 = #subl{},
            B0#subl{bc=bc_append(B0#subl.bc, {subclaster_blockchain, 
                                              Name, 
                                              {Type, InitKeyChain}}),
                    i_key=H};
        _ ->
            cannot_verify_chain
    end.


sb_append_admin2(Sb=#subl{bc=Blockchain, admins=Admins}, 
                 Admin={admin, _, _, PubKey}, Sig) ->
    case dict:is_key(PubKey, Admins) of
        false ->
            A1 = dict:append(PubKey, Admin, Admins),
            B1 = bc_append(Blockchain, {Admin, Sig}),
            Sb#subl{bc=B1, admins=A1};
        _ ->
            admin_exists
    end.

sb_check_majority(Admins, Signatures, Rec) when is_list(Signatures)->
    MajorityIs = round(dict:size(Admins) / 2 + 0.6),
    S2 = lists:usort(Signatures),
    S3 = [Si || Si={signature, PubKey, _} <- S2, 
                dict:is_key(PubKey, Admins)],
    case length(S3) < MajorityIs of
        true ->
            false;
        _ ->
            sig_verify(Rec, S3)
    end;
sb_check_majority(_, _, _) ->
    false.


sb_append_admin(Sb=#subl{bc=Blockchain, admins=Admins, o_nodes=[],
                     i_key=SPK0}, 
                Admin={admin, _, _, PubKey}, 
                Signature={signature, SPK, _}) when SPK == SPK0 ->
    case sig_verify(Admin, Signature) of
        true ->
            sb_append_admin2(Sb, Admin, Signature);
        _ ->
            cannot_verify_signature
    end;
sb_append_admin(Sb=#subl{bc=Blockchain, admins=Admins, o_nodes=ONodes}, 
                Admin={admin, _, _, PubKey}, 
                Signatures) 
  when is_list(Signatures), ONodes =/= [], is_list(ONodes) == true ->
    case sb_check_majority(Admins, Signatures, Admin) of
        true ->
            sb_append_admin2(Sb, Admin, Signatures);
        _ ->
            cannot_verify_signature2
    end;
sb_append_admin(#subl{bc=_Blockchain, admins=Admins, o_nodes=_ONodes,
                     i_key=SPK0}, Admin, Signature) ->
    case Admin of
        {admin, _, _, _PubKey} ->
            case is_list(Signature) of
                false ->
                    case Signature of
                        {signature, SPK, _} ->
                            case (SPK == SPK0) and (Admins =/= []) of
                                false ->
                                    unknown_error;
                                true ->
                                    initialization_finished
                            end;
                        _ ->
                            not_a_signature
                    end;
                true ->
                    % TODO: elaborate
                    probably_bad_multiple_signatures
            end;
        _ ->
            not_an_admin
    end.

sb_get_key(#subl{i_key=Key}) ->
    Key.

sb_get_admins(#subl{admins=Admins}) ->
    dict:fetch_keys(Admins).


sb_append_transaction(SB=#subl{bc=Blockchain, admins=Admins, 
                            o_nodes=ONodes}, 
                      ConsensedTransaction=
                          {
                           {
                            {
                             Data={ordering_node, OPK, _}, 
                             Sig={signature, PublicKey, _}
                            }, 
                            _Timestamp, _SeqNum}, 
                           Sig2
                          }) ->
    % first ordering node is owner of this blockchain
    % TODO: check if signing ordering node is correct and Sig2 is 
    % correct
    % TODO: verify also if endpoint and ordering node public key are
    % not used already, separately
    case lists:member(Data, ONodes) of
        false ->
            case dict:is_key(PublicKey, Admins) of
                true ->
                    case sig_verify(Data, Sig) of
                        true ->
                            Blockchain2 = bc_append(Blockchain, 
                                                    ConsensedTransaction),
                            SB#subl{bc=Blockchain2, 
                                    o_nodes=[Data|ONodes]};
                        _ ->
                            bad_signature
                    end;
                _ ->
                    unknown_admin
            end;
        _ ->
            ordering_node_exists
    end;
sb_append_transaction(SB=#subl{bc=Blockchain, admins=Admins, 
                            o_nodes=ONodes}, 
                      ConsensedTransaction=
                          {
                            Data={ordering_node, OPK, _}, 
                            Sig={signature, PublicKey, _}
                          }) ->
    % TODO: refactor
    case lists:member(Data, ONodes) of
        false ->
            case dict:is_key(PublicKey, Admins) of
                true ->
                    case sig_verify(Data, Sig) of
                        true ->
                            Blockchain2 = bc_append(Blockchain, 
                                                    ConsensedTransaction),
                            SB#subl{bc=Blockchain2, 
                                    o_nodes=[Data|ONodes]};
                        _ ->
                            bad_signature
                    end;
                _ ->
                    unknown_admin
            end;
        _ ->
            ordering_node_exists
    end.


    
sb_get_all_endpoints(_) ->
    todo.

sb_validate() ->
    todo.

sb_remove_admin() ->
    todo.


%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% gradido blockchain

% blockchain structure
% - header
%   - alias (name)
%   - initial key type (ca_signed, self_signed)
%   - list of public keys, representing CA chain (if type is ca_signed,
%     chain is verified with CA_MASTER_PUB, which is hardcoded)
%   - public key of ordering node
% - public keys of initial admins
%   - public key
%   - signed with initial key
% - public keys of initial nodes
%   - type (login, gradido)
%   - public key
%   - signed with initial key
% - initialization done transaction; after this point nothing is signed
%   with initialization key
%
% afterwards, transactions and node configuration events are present;
% any admin in subcluster blockchain can sign those; they have to
% match contents of subcluster blockchain
%
% actually, all nodes and admins can be represented by key only; type
% will be deducted from sb

    
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% nodes

% typical launch process of a node
% - node is started, providing AdminKeyPub to it
%   - additionaly, for ordering nodes not being primary for sb, ordering
%     node endpoint is provided as well
% - it creates its own key pair (PrivKey, PubKey)
% - it stands ready for incoming request for its public key
%   - request has been signed by AdminKeyPriv
%   - it includes timestamp, to not allow replay attacks
% - it returns its PubKey, signed by PrivKey
% - it waits for signed response, by AdminKeyPriv
% - for ordering node
%   - it puts record into blockchain, registering itself
% - for non-ordering node
%   - it sends this record to ordering node
%   - record is inserted in subcluster blockchain by ordering node
%   - node gets sb data from ordering node


% TODO: serve pings
nd_start_node(P={handshake0, AdminPubKey, NodeFunc, FuncData}) ->
    receive 
        M={Data={{handshake0, AdminEndpoint}, _Timestamp},
           Signature={signature, AdminPubKey, _}} ->
            case sig_verify(Data, Signature) of
                false ->
                    erlang:display({nd_start_node, warning, 
                                    bad_signature, M}),
                    nd_start_node(P);
                true ->
                    erlang:display({nd_start_node, info, handshake0}),
                    Kp = {key_pair, _, PubKey} = sig_create_pk_pair(),
                    Response = {handshake1, PubKey},
                    ep_send(AdminEndpoint, Response, Kp),
                    nd_start_node({handshake2, Kp, AdminPubKey,
                                            NodeFunc, FuncData})
            end;
        Any ->
            erlang:display({nd_start_node, warning, bad_message, Any}),
            nd_start_node(P)
    end;
nd_start_node(P={handshake2, Kp={key_pair, _, PubKey}, AdminPubKey, 
                 NodeFunc, FuncData}) ->
    receive 
        M={Data={{handshake2, SignedTransaction}, _Timestamp},
           Signature={signature, AdminPubKey, _}} ->
            case sig_verify(Data, Signature) of
                false ->
                    erlang:display({nd_start_node, warning, 
                                    bad_signature2, M}),
                    nd_start_node(P);
                true ->
                    erlang:display({nd_start_node, info, handshake2}),
                    {Module, Func} = NodeFunc,
                    Module:Func({init, FuncData, Kp, SignedTransaction})
            end;
        Any ->
            erlang:display({nd_start_node, warning, bad_message2, Any}),
            nd_start_node(P)
    end;
nd_start_node(Any) ->
    erlang:display({nd_start_node, error, bad_params, Any}).



-record(node, {
           kp,
           sb
          }).
-record(topic, {
           seq_num=1,
           running_hash=0
          }).
-record(onod, {
           topics
          }).

nd_assign_consensus(Tr, PrivKey, SeqNum) ->
    Tr2 = {Tr, msc_timestamp(), SeqNum},
    {SeqNum + 1, sig_sign_packet(Tr2, PrivKey)}.

nd_notify_all([], _, _) ->
    ok;
nd_notify_all([H|T], Data, PrivKey) ->
    ep_send(H, Data, PrivKey),
    nd_notify_all(T, Data, PrivKey).


nd_ordering_node({init, SB, Kp, SignedTransaction}) ->
    SB2 = sb_append_transaction(SB, SignedTransaction),
    Topics = dict:store(sb_get_key(SB2), #topic{}, dict:new()),
    nd_ordering_node({#node{kp=Kp, sb=SB2}, #onod{topics=Topics}});
nd_ordering_node({Node=#node{sb=SB, kp=Kp}, OrdNode=#onod{topics=Topics}}) ->
    erlang:display(listening),
    receive
        M={{do_order, BlockchainId, Data}, Signature} ->
            % TODO: check if sender is allowed to post to blockchain
            case dict:is_key(BlockchainId, Topics) of
                true ->
                    erlang:display({nd_ordering_node, info, do_order, M}),

                    % TODO: depending on blockchain type, should find
                    % out which nodes should receive notification; for
                    % now, all are notified
                    Topic = dict:fetch(BlockchainId, Topics),
                    SeqNum = Topic#topic.seq_num,
                    {SeqNum2, Data2} = nd_assign_consensus(
                                         Data, Kp, SeqNum),
                    nd_notify_all(sb_get_all_endpoints(SB), Data2, Kp),
                    Topic2 = Topic#topic{seq_num=SeqNum2},
                    Topics2 = dict:store(BlockchainId, Topic2, 
                                         Topics),
                    nd_ordering_node({Node, OrdNode#onod{
                                              topics=Topics2}});
                false ->
                    erlang:display({nd_ordering_node, warning, 
                                    bad_blockchain, M}),
                    nd_ordering_node({Node, OrdNode})
            end;
        M={{sb_append_transaction, ConsensedTransaction}, Signature} ->
            erlang:display({nd_ordering_node, info, 
                            sb_append_transaction, M}),
            % TODO: verify signature
            SB2 = sb_append_transaction(SB, ConsensedTransaction),
            nd_ordering_node({Node#node{sb=SB2}, OrdNode});
        {{finish, _Timestamp}, Signature} ->
            erlang:display({nd_ordering_node, info, finish, 
                            Node, OrdNode}),
            ok;
        Any ->
            erlang:display({nd_ordering_node, warning, bad_message, 
                            Any}),
            nd_ordering_node({Node, OrdNode})
    end.





%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% tests

test_sig() ->
    ok = sig_init(),
    {key_pair, Priv, Pub} = sig_create_pk_pair(),
    Data = {"some data", some_atom, 123},
    SignedData = sig_sign(Data, Priv),
    {signature, Pub, _} = SignedData,
    true = sig_verify(Data, SignedData),

    {key_pair, _Dpr, Dpu} = sig_derive(Priv),
    true = sig_is_derived(Pub, Dpu),

    C2 = sig_derive(?CA_MASTER),
    C1 = sig_derive(C2),
    C0 = sig_derive(C1),
    C = [Pub2 || {key_pair, _Priv2, Pub2} <- [C0, C1, C2]],
    true = sig_verify_chain(C),

    true = sig_verify(Data, [sig_sign(Data, C0), 
                             sig_sign(Data, C1),
                             sig_sign(Data, C2)]),

    ok.

test_bc() ->
    B = bc_create(),
    B2 = bc_append(B, {"piece of data 0", 123}),
    B3 = bc_append(B2, {"piece of data 1", 1234, {ha}}),
    B4 = bc_append(B3, {"piece of data 2", 12345, {haha}}),
    B5 = bc_append(B4, {"piece of data 3", 12345, {hahaha}}),
    B6 = bc_append(B5, {"piece of data 4", 12345, {hahaha}}),
    B7 = bc_append(B6, {"piece of data 5", 12345, {hahaha}}),
    B8 = bc_append(B7, {"piece of data 6", 12345, {hahaha}}),
    B9 = bc_append(B8, {"piece of data 7", 12345, {hahaha}}),
    B10 = bc_append(B9, {"piece of data 8", 12345, {hahaha}}),
    B11 = bc_append(B10, {"piece of data 9", 12345, {hahaha}}),

    checksums_match = bc_verify_checksum(B11),
    B12 = bc_extract_checksums(B11),
    {checksums,[{checksum,30298732},{checksum,36246981}],12} = B12,
    ok.

test_create_sb() ->
    {key_pair, InitPriv2, InitPub2} = sig_derive(?CA_MASTER),
    {key_pair, InitPriv1, InitPub1} = sig_derive(InitPriv2),

    Res = {key_pair, AdminPriv1, AdminPub1} = sig_create_pk_pair(),
    {key_pair, AdminPriv2, AdminPub2} = sig_create_pk_pair(),
    {key_pair, AdminPriv3, AdminPub3} = sig_create_pk_pair(),

    Admin1 = {admin, "email1", "name1", AdminPub1},
    Sig1 = sig_sign(Admin1, InitPriv1),
    Admin2 = {admin, "email2", "name2", AdminPub2},
    Sig2 = sig_sign(Admin2, InitPriv1),
    Admin3 = {admin, "email3", "name3", AdminPub3},
    Sig3 = sig_sign(Admin3, InitPriv1),

    SB0 = sb_create(self_signed, [InitPub1, InitPub2], "testing"),
    SB1 = sb_append_admin(SB0, Admin1, Sig1),
    SB2 = sb_append_admin(SB1, Admin2, Sig2),
    SB3 = sb_append_admin(SB2, Admin3, Sig3),
    {Res, SB3}.

test_sb() ->
    test_create_sb(),
    % TODO: some checks
    ok.

test_nd() ->
    {Kp={key_pair, AdminPriv, AdminPub}, SB} = test_create_sb(),

    Self = ep_create(),

    Orn = ep_spawn(?MODULE, nd_start_node, {handshake0, AdminPub,
                                            {?MODULE, nd_ordering_node},
                                            SB}),
    timer:sleep(100),
    H0 = {handshake0, Self},
    ep_send(Orn, H0, Kp),

    {Data1={{handshake1, PubKey1}, Timestamp1}, Signature1} = 
       ep_receive(),
    true = sig_verify(Data1, Signature1),
    SignedTransaction = sig_sign_packet({ordering_node, PubKey1, Orn}, 
                                        Kp),
    H2 = {handshake2, SignedTransaction},
    ep_send(Orn, H2, Kp),
    timer:sleep(100),

    ep_send(Orn, finish, Kp),

    timer:sleep(100).
    

test() ->
    test_sig(),
    test_bc(),
    test_sb(),
    test_nd().
