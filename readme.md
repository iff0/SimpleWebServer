###  SimpleWebServer 

[![Static Badge](https://img.shields.io/badge/license-MIT-blue?logo=git&logoColor=%20)]() [![Static Badge](https://img.shields.io/badge/C%2B%2B-17-green?logo=cplusplus&logoColor=blue)]() [![Static Badge](https://img.shields.io/badge/cmake-3.22%2B-green?logo=cmake&logoColor=deepskyblue)]() [![Static Badge](https://img.shields.io/badge/compiler-gcc11.3%2B-green?logo=compilerexplorer)]()  [![Static Badge](https://img.shields.io/badge/Linux-18.04%20LTS-green?logo=linux&logoColor=%20)]() [![Static Badge](https://img.shields.io/badge/github-fmt10.1.0-blue?logo=github&logoColor=%20&link=https%3A%2F%2Fgithub.com%2Ffmtlib%2Ffmt)](https://github.com/fmtlib/fmt)

---


一个单`Reactor`与工作线程池模型的静态网站服务端，仅支持`GET`请求的解析，支持长连接复用套接字，定时清除非活跃长连接

代码参考：[![Static Badge](https://img.shields.io/badge/github-TinyWebServer-blue?logo=github&logoColor=%20&link=https%3A%2F%2Fgithub.com%2Fqinguoyi%2FTinyWebServer)](https://github.com/qinguoyi/TinyWebServer)、游双《linux高性能服务器》

#### Usage 

```bash
mkdir build
cd build 
cmake .. 
make 
./cpp_simple_web_server ip_address port_number threads_num [index_page_path]
```

其中`ip_address`为服务器局域网地址，`port_number`为监听端口，`threads_num`为工作线程数，`index_page_path`为网站根目录，默认为`/var/www/html`

经`Webbench`测试，设定工作线程数为`10`时，`QPS`可达`5.2w`左右

![](https://raw.githubusercontent.com/zixfy/imageshack2023/main/img/e4b632288a76913c9e90adf94f60dca.png)

