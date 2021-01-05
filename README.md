# tls-project-remote-shell

## 建置環境
ubuntu 18.04，g++-8，openssl

## 建置方法
`g++-8-std=c++17 -O3 https_server.cpp -o https_server -lssl -lcrypto -lstdc++fs`

## 執行方法
`./https_server`

可在server.cfg改port。
