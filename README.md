# malshin

## purpose
The purpose of malshin is to be a container tool.
You should use malshin to create and run isloated containers.

## usage
### compilation
```bash
./compile.sh
```

### running
```bash
./malshin
```

## features
Currently it doesn't contain all the isolations it should.

The following isolations are working:
- mounts
- pids
- IPC
- UTS
- file descriptors

The following isolations are partly working:
- network
- control groups
