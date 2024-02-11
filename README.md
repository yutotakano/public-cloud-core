# PubliCore

To clone dependencies and create the executable:

```shell
git clone git@github.com:yutotakano/public-cloud-core.git
cd public-cloud-core
make
```

To run:

```shell
./publicore --help
```

To clean, use either of:

```shell
make clean # clean only project build artifacts
make clean_all # clean including dependencies (will trigger a clone next run)
```
