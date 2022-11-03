@echo off

protoc -I=./ --cpp_out=./ ./message.proto

del message.pb.cpp
ren message.pb.cc message.pb.cpp

