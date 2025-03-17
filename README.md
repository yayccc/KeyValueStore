# Key Value Store

## 简介

该项目是一个客户端可以远程连接服务器，并存储键值对的服务端组件

允许`SET GET MOD DEL`基本命令

包含以下组件

网络基本模块

`map`存储，使用哈希表的方式存储键值对

相关协议解析模块

`WAL`持久化，允许服务端终止再启动时，从日志中恢复数据

## 存储引擎
目前以哈希表的方式存储(`cpp`的`unordered_map`)

后续会支持更多高性能的存储结构


## 持久化

目前以`WAL`的方式存入日志文件

后续会支持`LSM-Tree`的方式来允许海量数据的存储和查询

## 网络模块
并发能力较弱，会不断更新以提高网络并发能力

已增加线程池，可以并发执行io任务

封装了Reactor，使结构更清晰，提供运行，方便后续更改架构

## 协议
对于KVStore的命令，更新为Resp协议,支持批量操作
例如：
```plain
*15\r\n
$3\r\nSET\r\n$4\r\nkey1\r\n$6\r\nvalue1\r\n
$3\r\nSET\r\n$4\r\nkey2\r\n$6\r\nvalue2\r\n
$3\r\nMOD\r\n$4\r\nkey1\r\n$8\r\n"ababab"\r\n
$3\r\nDEL\r\n$4\r\nkey2\r\n
$3\r\nGET\r\n$4\r\nkey1\r\n
$3\r\nGET\r\n$4\r\nkey2\r\n
```
进行了批量操作，包含两次SET，两次GET，一次MOD，一次DEL