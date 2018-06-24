# Kaplar

## About
This is hobby project. I'm aiming to remake the original OpenTibia Server (initially for client version 8.6) using C, C++, and Lua languages extensively. As the original project kept introducing features but in some cases not properly implementing them (which made the code base somewhat inconsistent), i felt i could try to remake the whole thing from scratch, optimizing and improving the functionality on the way.

## License
I chose the MIT License because it is a simple permissive license.
Refer to LICENSE in the root directory for details.

## Project Layout
```
data/           data folder
docs/           couple notes
src/            source code
test/           unit tests
vc15/           Visual Studio 15
```

## Systems
A server is made up of various systems and before i begin implementation of the Tibia protocol, i must be certain that core systems are ready. Such systems as configuration, networking, scripting, and database.

The status for these systems are listed below:
```
[x] configuration
[ ] scripting
[+] networking
[x]     test protocol
[+] database
[ ]     Local
[ ]     CassandraDB
[ ]     PostgreSQL
```
