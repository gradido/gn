curl -X POST \
     -H 'Content-Type: application/json' \
     -d '{"jsonrpc":"2.0","id":"id","method":"getTransactions","params":{"groupAlias":"aa","lastKnownSequenceNumber":1}}' \
     http://localhost:13001


