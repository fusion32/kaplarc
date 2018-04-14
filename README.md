# Kaplar

## About
This is hobby project. I'm aiming to remake the original OpenTibia Server (initially for client version 8.6) using C, C++, and Lua languages extensively. As the original project kept introducing features but in some cases not properly implementing them (which made the code base somewhat inconsistent), i felt i could try to remake the whole thing from scratch, optimizing and improving the functionality on the way.

## License
I chose the MIT License because it is a simple permissive license.
Refer to LICENSE inside the project directory for details.

## Project Layout
```
data/           data folder
src/            source
test/		unit tests
vc15/           Visual Studio 15
```

## Systems
A server is made up of various systems and before i begin implementation of the Tibia protocol, i must be certain that core systems are ready. Such systems as configuration, networking, scripting, and database.

The status for these systems are listed below:
```
[x] configuration
[x] networking protocols
[ ] scripting
[+] database
[ ]	MongoDB backend
[ ]	Cassandra backend
```

REMARK: for the official database backend, i'm still divided between MongoDB and Cassandra. Both have appealing features but i'm leaning towards MongoDB simply because it's developed in C++ and the connectors are provided by the same guys. Whereas Cassandra is developed in Java and it's connectors are distributed from a third-party company.

## Contributors

- fusion32

> **Note:** Anyone is welcome to contribute with the project.
