# NOTE: in order to launch, should prepare hedera-credentials.cfg file
# in this folder, check hedera-credentials.template.cfg

export EXTENSIVE_LOGGING_ENABLED=1

# uncomment outter cycle in order for "reset all" feature to work inside
# webapp

#while :
#do
#    python ncd.py ncd.yaml case=brisbane hedera-topic-id=0,0,17023
#    python ncd.py ncd.yaml case=perth hedera-topic-id=0,0,17023
    python ncd.py ncd.yaml case=canberra hedera-group-topic-id=0,0,79574 hedera-topic-id-pool=79606,79607,79608,79609,79610
    sleep 1
#done
