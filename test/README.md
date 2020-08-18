- steps to launch:
  - these are black box tests; python package named pukala is used for
    them; in essence, a data tree is built during tests which can
    later be verified automatically or read for reference
  - ensure python2.7 and pip is installed on the system
  - install requirements with
    pip install -r requirements.txt
  - create hedera account
  - inside test folder do
    cp hedera-credentials.template.cfg hedera-credentials.cfg
  - update hedera-credentials.cfg with hedera account data, maybe 
    setting its read access to current user only (as it contains private
    key)
  - create hedera topic with given account
  - use following command to start test case
    python ncd.py ncd.yaml case=gradido-configured-transfer hedera-topic-id=<topic id in form x,y,z>
  - check test-stage folder for test output
    - ncd-output.yaml contains result of growing pukala tree
    - ncd-output-steps.yaml contains part of the tree, for convenience
      - this part describes actual test steps, in this case (step by
        step):
        - generating gradido node configuration (file and blockchain
          folders) and launching it
        - creating a request normally sent by login server, with 
          transaction data
        - taking request data from previous step, translating it into
          GRPC request, feeding it to hedera
        - sleeping for 10 seconds to allow hedera to reach consensus,
          gradido node to receive and process transaction
        - stopping gradido node
          - this step involves dumping of stderr from gradido node and
            blockchain data
    - gradido-node-0 contains:
      - err-output.txt: stderr of gradido node process
      - gradido.conf: used by gradido node
      - t_blocks.1.<topic_id in form x.y.z>.bc: folder with blockchain
        data
- it is possible to check and replicate test conditions (there is git
  commit hash in ncd-output.yaml, along with test timestamp, and other 
  useful data)
