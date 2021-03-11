Gradido Node v1 is a node which stores Gradido Blockchain. This 
repository contains other node types (ordering, backup, etc.) as well 
for v2.

### Docker

When code is built, all node types + unit tests + utilites are accessible
as executables. It is expected that a docker container runs single binary
(1 container = 1 node).

Ports 13000-14000 all are exposed; exact port should be specified in
configuration file, provided inside volume, which serves also as home
folder for particular instance.

#### env:

$INSTANCE_FOLDER:   home of instance; should contain gradido.conf, 
                    siblings.txt; note, that folder is referred from
                    inside of gradido.conf as well
$INSTANCE_NAME:     arbitrary name of instance

#### build:

> docker build -t $INSTANCE_NAME .

#### run deprecated (v1) gradido node:

> docker run --ulimit core=-1 -v $INSTANCE_FOLDER:/opt/instance $INSTANCE_NAME /opt/gn/build/gradido_deprecated


#### run tests:

> docker run --ulimit core=-1 $INSTANCE_NAME /opt/gn/build/unit_tests

--ulimit core=-1 should result in core files if node crash
