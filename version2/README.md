## 版本二

### 目标
针对第一版出现的坑点和功能进行改进。  

### 坑点
1.数据传输要在json 之前加上length。  
。。。等待补充  

### 功能
1.server采用Reactor模式，使用半同步---半异步线程池处理业务逻辑。  
2.使用MySQL保存账户和密码  
3.使用redis保存单词--汉译（暂时）  
4.可以查询热点词汇（New）  
5.可以自我检测，背词（New）  
