#include "httpd.h"


namespace httpd
{

/*------------implement of utils--------------*/
namespace utils
{

const std::string toLower(const std::string& str){
	std::string res;
	for (auto i:str){
		res.push_back(std::tolower(i));
	}
	return res;
}

const std::shared_ptr<std::string> urlDecode(const std::shared_ptr<std::string> input) {
    if (nullptr==input) return nullptr;
    std::string decoded;
    for (size_t i = 0; i < input->length(); ++i) {
        if ((*input)[i] == '%' && i + 2 < input->length()) {
            // 读取%后的两个字符，解析为16进制数，然后转换为字符
            char hex1 = (*input)[i + 1];
            char hex2 = (*input)[i + 2];
            if (isxdigit(hex1) && isxdigit(hex2)) {
                char decodedChar = static_cast<char>(std::stoi(input->substr(i + 1, 2), 0, 16));
                decoded += decodedChar;
                i += 2;
            }
        } else if ((*input)[i] == '+') {
            // 将+号替换为空格
            decoded += ' ';
        } else {
            // 直接添加其他字符
            decoded += (*input)[i];
        }
    }
    return std::make_shared<std::string>(decoded);
}

const std::shared_ptr<std::string> urlEncode(const std::shared_ptr<std::string> input){
	//todo
	return input;
}


} // namespace utils

/*------------implement of Method--------------*/
Method::Method(const Method::Type& type){
	this->type=type;
}
Method::Method(const std::shared_ptr<std::string> str){
	if ("GET"==*str){
		this->type=Method::Type::GET;
	}
	else if ("POST"==*str){
		this->type=Method::Type::POST;
	}
	else {
		throw HttpException(StatusCodeAndMessage::Type::BadRequest);
	}
}

Method::Type Method::getType() const{
	return this->type;
}
std::shared_ptr<std::string> Method::toString() const{
	if (Method::Type::GET==this->type) return std::make_shared<std::string>("GET");
	if (Method::Type::POST==this->type) return std::make_shared<std::string>("POST");
	throw HttpException(StatusCodeAndMessage::Type::BadRequest);
}
bool Method::operator==(const Method& cmp) const{
	return this->type==cmp.type;
}
bool Method::operator==(const Method::Type& cmp) const{
	return this->type==cmp;
}
bool Method::operator!=(const Method& cmp) const{
	return this->type!=cmp.type;
}
bool Method::operator!=(const Method::Type& cmp) const{
	return this->type!=cmp;
}

/*------------implement of Version--------------*/
Version::Version(const Version::Type& type){
	this->type=type;
}
Version::Version(const std::shared_ptr<std::string> str){
	if ("HTTP/1.0"==*str){
		this->type=Version::Type::HTTP_1_0;
	}
	else if ("HTTP/1.1"==*str){
		this->type=Version::Type::HTTP_1_1;
	}
	else {
		throw HttpException(StatusCodeAndMessage::Type::BadRequest);
	}
}

Version::Type Version::getType() const{
	return this->type;
}
std::shared_ptr<std::string> Version::toString() const{
	if (Version::Type::HTTP_1_0==this->type) return std::make_shared<std::string>("HTTP/1.0");
	if (Version::Type::HTTP_1_1==this->type) return std::make_shared<std::string>("HTTP/1.1");
	throw HttpException(StatusCodeAndMessage::Type::BadRequest);
}
bool Version::operator==(const Version& cmp) const{
	return this->type==cmp.type;
}
bool Version::operator==(const Version::Type& cmp) const{
	return this->type==cmp;
}
bool Version::operator!=(const Version& cmp) const{
	return this->type!=cmp.type;
}
bool Version::operator!=(const Version::Type& cmp) const{
	return this->type!=cmp;
}

/*------------implement of StatusCodeAndMessage--------------*/
StatusCodeAndMessage::StatusCodeAndMessage(const StatusCodeAndMessage::Type& type){
	this->type=type;
}

StatusCodeAndMessage::Type StatusCodeAndMessage::getType() const{
	return this->type;
}
std::shared_ptr<std::string> StatusCodeAndMessage::toString() const{
	if (StatusCodeAndMessage::Type::Continue==this->type) return std::make_shared<std::string>("100 Continue");
	if (StatusCodeAndMessage::Type::OK==this->type) return std::make_shared<std::string>("200 OK");
	if (StatusCodeAndMessage::Type::BadRequest==this->type) return std::make_shared<std::string>("400 BadRequest");
	if (StatusCodeAndMessage::Type::Unauthorized==this->type) return std::make_shared<std::string>("401 Unauthorized");
	if (StatusCodeAndMessage::Type::Forbidden==this->type) return std::make_shared<std::string>("403 Forbidden");
	if (StatusCodeAndMessage::Type::NotFound==this->type) return std::make_shared<std::string>("404 NotFound");
	if (StatusCodeAndMessage::Type::InternalServerError==this->type) return std::make_shared<std::string>("500 InternalServerError");
	return std::make_shared<std::string>("0 UNKNOW");
}
bool StatusCodeAndMessage::operator==(const StatusCodeAndMessage& cmp) const{
	return this->type==cmp.type;
}
bool StatusCodeAndMessage::operator==(const StatusCodeAndMessage::Type& cmp) const{
	return this->type==cmp;
}
bool StatusCodeAndMessage::operator!=(const StatusCodeAndMessage& cmp) const{
	return this->type!=cmp.type;
}
bool StatusCodeAndMessage::operator!=(const StatusCodeAndMessage::Type& cmp) const{
	return this->type!=cmp;
}

/*------------implement of Body--------------*/
Body::Body(const std::shared_ptr<std::string> type, const std::shared_ptr<std::vector<unsigned char>> content){
	this->sp_type=type;
	if (type->find("text/")!=type->npos) { //Content-Type是文本类型就设置字符类型为utf-8
		*(this->sp_type)+="; charset=utf-8";
	}
	this->sp_content=content;
}

const std::shared_ptr<std::string> Body::getType() const{
	return this->sp_type;
}
const std::shared_ptr<std::vector<unsigned char>> Body::getContent() const{
	return this->sp_content;
}


/*------------implement of HttpException--------------*/
HttpException::HttpException(const StatusCodeAndMessage::Type& type):sp_status_code_and_msg(std::make_shared<StatusCodeAndMessage>(type)){}
const char* HttpException::what() const throw() {
	return this->sp_status_code_and_msg->toString()->c_str();
}

const std::shared_ptr<StatusCodeAndMessage> HttpException::getStatusCodeAndMessage() const{
	return this->sp_status_code_and_msg;
}

/*------------implement of Request--------------*/
Request::Request(){
	this->sp_version=std::make_shared<Version>(Version::Type::HTTP_1_1); // 默认使用HTTP/1.1
	this->sp_headers=std::make_shared<std::unordered_map<std::string,std::shared_ptr<std::string>>>();
}

// 将字符串解析为Request对象
void Request::decode(const std::shared_ptr<std::string> str){
	try
	{
		std::string request_str=*(utils::urlDecode(str)); //需要先对request字符串进行url解码
		std::size_t pos;
		//解析初始行
		pos=request_str.find_first_of("\r\n"); //找到\r\n中的任意一个
		{
			auto tmp=Request::parseInitiaLine(request_str.substr(0,pos));
			this->sp_method=std::make_shared<Method>(std::make_shared<std::string>(std::get<0>(tmp)));
			this->sp_path=std::make_shared<std::string>(std::get<1>(tmp));
			this->sp_version=std::make_shared<Version>(std::make_shared<std::string>(std::get<2>(tmp)));
		}

		//切换到下一行，需要处理以\r换行、\n换行或者\r\n换行这三种情况
		if ('\n'==request_str[pos+1]||'\r'==request_str[pos+1]) request_str=request_str.substr(pos+2); 
		else request_str=request_str.substr(pos+1);

		//解析headers
		while(1){
			pos=request_str.find("\r\n");
			if (0==pos) break;
			if (request_str.npos==pos) throw HttpException(StatusCodeAndMessage::Type::BadRequest);
			auto tmp=Request::parseKeyValuePairLine(request_str.substr(0,pos));
			this->setHeader(std::make_shared<std::string>(tmp.first),std::make_shared<std::string>(tmp.second));
			//切换到下一行
			if ('\n'==request_str[pos+1]||'\r'==request_str[pos+1]) request_str=request_str.substr(pos+2);
			else request_str=request_str.substr(pos+1);
		}

		//解析body
		if ('\n'==request_str[pos+1]||'\r'==request_str[pos+1]) request_str=request_str.substr(pos+2);
		else request_str=request_str.substr(pos+1);
		if (0!=request_str.length()){
			auto value=this->getHeader(std::make_shared<std::string>("content-type"));
			if (nullptr==value){
				this->setBody(std::make_shared<Body>(
					std::make_shared<std::string>("text/plain"),
					std::make_shared<std::vector<unsigned char>>(request_str.begin(),request_str.end())
				));
			}
			else {
				this->setBody(std::make_shared<Body>(
					value,
					std::make_shared<std::vector<unsigned char>>(request_str.begin(),request_str.end())
				));
			}
		}
	}
	catch(const HttpException& e){
		std::cerr << e.what() << "in Request::decode\n";
		throw e;
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << "in Request::decode\n";
		throw e;
	}
}
const std::shared_ptr<std::string> Request::encode(){
	try
	{
		std::string str=*(this->sp_method->toString())+" "+*(utils::urlEncode(this->sp_path))+" "+*(this->sp_version->toString())+"\r\n";
		for (auto i:*(this->sp_headers)){
			str+=i.first+": "+*(i.second)+"\r\n";
		}
		str+="\r\n";
		if (nullptr!=this->getBody()) str+=*(utils::urlEncode(std::make_shared<std::string>(std::string(this->sp_body->getContent()->begin(),this->sp_body->getContent()->end()))));
		
		return std::make_shared<std::string>(str);
	}
	catch(const HttpException& e){
		std::cerr << e.what() << "in Request::encode\n";
		throw e;
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << "in Request::encode\n";
		throw e;
	}
	
}

void Request::setMethod(const Method::Type& type){
	this->sp_method=std::make_shared<Method>(type);
}
const std::shared_ptr<Method> Request::getMethod() const{
	return this->sp_method;
}
void Request::setPath(const std::shared_ptr<std::string> path){
	this->sp_path=path;
}
const std::shared_ptr<std::string> Request::getPath() const{
	return this->sp_path;
}
void Request::setVersion(const Version::Type& type){
	this->sp_version=std::make_shared<Version>(type);
}
const std::shared_ptr<Version> Request::getVersion() const{
	return this->sp_version;
}
void Request::setHeader(const std::shared_ptr<std::string> key,const std::shared_ptr<std::string> value){
	(*(this->sp_headers))[utils::toLower(*key)]=value;
}
const std::shared_ptr<std::string> Request::getHeader(const std::shared_ptr<std::string> key) const{
	if (0==this->sp_headers->count(utils::toLower(*key))) return nullptr; //使用count方法可以避免出现插入nullptr的情况
	return (*(this->sp_headers))[utils::toLower(*key)];
}
void Request::setBody(const std::shared_ptr<Body> body){
	this->sp_body=body;
	this->setHeader(std::make_shared<std::string>("content-type"),body->getType());
	this->setHeader(std::make_shared<std::string>("content-length"),std::make_shared<std::string>(std::to_string(body->getContent()->size())));
}
const std::shared_ptr<Body> Request::getBody() const{
	return this->sp_body;
}

std::tuple<std::string,std::string,std::string> Request::parseInitiaLine(const std::string& line){
	std::tuple<std::string,std::string,std::string> res;
	std::istringstream line_stream(line);
	line_stream>>std::get<0>(res)>>std::get<1>(res)>>std::get<2>(res);
	return res;
}

std::string Request::trimWhitespace(const std::string& str) {
	std::size_t pos1=str.find_first_not_of(" \t");
	std::size_t pos2=str.find_last_not_of(" \t");
	return str.substr(pos1,pos2-pos1+1);
}

std::pair<std::string,std::string> Request::parseKeyValuePairLine(const std::string& line) {
	std::pair<std::string,std::string> kv_pair;
	std::string tmp=Request::trimWhitespace(line);
	auto pos=tmp.find(':');
	if (tmp.npos==pos) throw HttpException(StatusCodeAndMessage::Type::BadRequest);
	kv_pair.first=Request::trimWhitespace(tmp.substr(0,pos));
	kv_pair.second=Request::trimWhitespace(tmp.substr(pos+1));
	return kv_pair;
}

/*------------implement of Response--------------*/
Response::Response(){
	// 默认使用HTTP/1.1
	this->sp_version=std::make_shared<Version>(Version::Type::HTTP_1_1);
	this->sp_status_code_and_msg=std::make_shared<httpd::StatusCodeAndMessage>(httpd::StatusCodeAndMessage::Type::OK);
	this->sp_headers=std::make_shared<std::unordered_map<std::string,std::shared_ptr<std::string>>>();
}

const std::shared_ptr<std::string> Response::encode() {
	// 将Response对象转为文本内容
	std::string str=*(this->sp_version->toString())+" "+*(this->sp_status_code_and_msg->toString())+"\r\n";
	for (auto i:*(this->sp_headers)){
		str+=i.first+": "+*(i.second)+"\r\n";
	}
	str+="\r\n";
	if (nullptr!=this->getBody()) str+=std::string(this->sp_body->getContent()->begin(),this->sp_body->getContent()->end());
	return std::make_shared<std::string>(str);
}

void Response::setVersion(const Version& version){
	this->sp_version=std::make_shared<Version>(version.getType());
}
const std::shared_ptr<Version> Response::getVersion() const{
	return this->sp_version;
}
void Response::setStatusCodeAndMessage(const std::shared_ptr<StatusCodeAndMessage> status_code_and_msg){
	this->sp_status_code_and_msg=status_code_and_msg;
}
const std::shared_ptr<StatusCodeAndMessage> Response::getStatusCodeAndMessage() const{
	return this->sp_status_code_and_msg;
}
void Response::setHeader(const std::shared_ptr<std::string> key,const std::shared_ptr<std::string> value){
	(*(this->sp_headers))[utils::toLower(*key)]=value;
}
const std::shared_ptr<std::string> Response::getHeader(const std::shared_ptr<std::string> key) const{
	if (0==this->sp_headers->count(utils::toLower(*key))) return nullptr;
	return (*(this->sp_headers))[utils::toLower(*key)];
}
void Response::setBody(const std::shared_ptr<Body> body){
	this->sp_body=body;
	this->setHeader(std::make_shared<std::string>("content-type"),body->getType());
	this->setHeader(std::make_shared<std::string>("content-length"),std::make_shared<std::string>(std::to_string(body->getContent()->size())));
}
const std::shared_ptr<Body> Response::getBody() const{
	return this->sp_body;
}
std::shared_ptr<Response> Response::quickBuild(const std::shared_ptr<StatusCodeAndMessage> status_code_and_msg){
	auto response=std::make_shared<Response>();
	auto scm_str=status_code_and_msg->toString();
	auto body=std::make_shared<httpd::Body>(
		std::make_shared<std::string>("text/plain"),
		std::make_shared<std::vector<unsigned char>>(scm_str->begin(),scm_str->end())
	);
	response->setStatusCodeAndMessage(status_code_and_msg);
	response->setBody(body); //Response一定要有body
	return response;
}

/*------------implement of FileSystem--------------*/
FileSystem::FileSystem(const std::shared_ptr<std::string> file_root){
	this->sp_file_root=std::make_shared<std::string>("./"+*file_root+"/"); //确保在程序运行目录下。多加几个'/'比较保险
}

const std::shared_ptr<Body> FileSystem::read(const std::shared_ptr<std::string> file_name) {
	try
	{
		if (!this->isAccessPermitted(file_name)) { //访问路径escape了
			std::cerr <<"Forbidden in FileSystem::read\n";
			throw HttpException(StatusCodeAndMessage::Type::Forbidden);
		}
		std::ifstream file(*(this->sp_file_root)+*file_name, std::ios::binary);
		if (!file.is_open()){
			std::cerr <<"NotFound in FileSystem::read\n";
			throw HttpException(StatusCodeAndMessage::Type::NotFound);
		}
		//处理文件类型
		std::string type;
		auto pos=file_name->find_last_of(".");
		if (file_name->npos==pos) type="text/plain";
		else {
			auto ext=file_name->substr(pos+1);
			if (0==this->mime_types.count(ext)) type="text/plain";
			else type=(this->mime_types)[ext];
		}
		//读取文件内容
		std::vector<unsigned char> content;
		content.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());

		file.close();
		return std::make_shared<Body>(std::make_shared<std::string>(type),std::make_shared<std::vector<unsigned char>>(content));
	}
	catch (const httpd::HttpException& e){
		throw e;
	}
	catch (const std::ifstream::failure& e) {
		std::cerr << e.what() <<" in FileSystem::read"<< std::endl;
		throw HttpException(StatusCodeAndMessage::Type::NotFound);
	}
}

bool FileSystem::isAccessPermitted(const std::shared_ptr<std::string> file_name) const{
	return file_name->npos==file_name->find("../");
}


/*------------implement of Rule--------------*/
Rule::Rule(const std::shared_ptr<std::string> sp_rule_str){
	std::istringstream iss(*sp_rule_str);
	std::string action, from, ip;
	iss>>action>>from>>ip;
	if ("from"!=from)  throw std::runtime_error("wrong rule found in Rule::Rule");
	if ("allow"==action) this->action=Action::ALLOW;
	else if ("deny"==action) this->action=Action::DENY;
	else throw std::runtime_error("wrong rule found in Rule::Rule");
	
	auto pos=ip.find("/");
	if (ip.npos==pos) throw std::runtime_error("wrong rule found in Rule::Rule");
	auto prefix_len=std::stoi(ip.substr(pos+1));
	if (prefix_len<0||prefix_len>32) throw std::runtime_error("wrong rule found in Rule::Rule");
	this->network.mask=0xFFFFFFFF;
	this->network.mask<<=(32-prefix_len); //计算掩模

	ip=ip.substr(0,pos);
	struct in_addr addr;
	if (inet_pton(AF_INET, ip.c_str(), &addr) != 1) throw std::runtime_error("wrong rule found in Rule::Rule");
	this->network.network=ntohl(addr.s_addr)&(this->network.mask); //计算子网
}

bool Rule::isAllow() const {
	return Action::ALLOW==this->action;
}

bool Rule::isMatch(const std::shared_ptr<std::string> sp_ip) const {
	struct in_addr addr;
	if (inet_pton(AF_INET, sp_ip->c_str(), &addr)!=1) return false;
	uint32_t ip_addr = ntohl(addr.s_addr);
	return (ip_addr & this->network.mask) == this->network.network;
}

/*------------implement of IPAccessControl--------------*/
IPAccessControl::IPAccessControl(const std::shared_ptr<std::string> sp_rule_file){
	std::ifstream file(*sp_rule_file);
	if (!file.is_open()) throw std::runtime_error("no rule file in IPAccessControl::IPAccessControl");
	std::string line;
	while (std::getline(file, line)) {
		try{
			Rule rule(std::make_shared<decltype(line)>(line));
			this->rules.push_back(rule);
		}
		catch(const std::exception& e){
			std::cerr << e.what() << '\n';
		}
	}
	file.close();
}

bool IPAccessControl::isAllow(const std::shared_ptr<std::string> sp_ip) const{
	for (const auto& rule:this->rules) {  //Only the first match is used
		if (rule.isMatch(sp_ip)) return rule.isAllow();
	}
	return false;
}


/*------------implement of Server--------------*/
Server::Server(const int port, const size_t pool_size, const std::shared_ptr<std::string> sp_rule_file){
	this->server_fd = socket(AF_INET,SOCK_STREAM,0);
	struct sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;
	socklen_t addrlen = sizeof(addr);
	if(bind(this->server_fd,(sockaddr*)&addr,addrlen)) throw std::runtime_error("bind failed in Server::Server");
	if(listen(this->server_fd,MAX_LISTEN_QUEUE_LEN)) throw std::runtime_error("listen failed in Server::Server");
	if (pool_size > 0) this->sp_pool=std::make_shared<::utils::ThreadPool>(pool_size); //开启线程池
	try{ //初始化IP访问控制对象
		this->sp_ip_access_control=std::make_shared<IPAccessControl>(sp_rule_file);
	}
	catch(const std::exception& e){
		this->sp_ip_access_control=nullptr; //初始化IP访问控制对象失败
		std::cerr << e.what() << '\n';
	}
}
Server::~Server(){
	close(this->server_fd);
}

void Server::setMessageCallback(std::function<std::shared_ptr<httpd::Response>(const std::shared_ptr<httpd::Request>)> callback){
	this->message_callback=std::move(callback);
}

void Server::run(){
	struct sockaddr_in client_addr;
	socklen_t ca_len = sizeof(client_addr);
	while(1){
		int client_fd = accept(this->server_fd, (struct sockaddr*)&client_addr, &ca_len);
		if (client_fd<0) throw std::runtime_error("accept failed in Server::run");
		try{
			this->sp_pool->addTask(std::bind(&Server::task,this,client_fd)); //添加任务到线程池中
		}
		catch(const std::exception& e){
			std::cerr << e.what() << '\n';
		}
	}
}

void Server::task(int client_fd){
	try{
		if (nullptr!=this->sp_ip_access_control){ //检查IP是否允许访问
			struct sockaddr_in client_addr;
			socklen_t addr_len = sizeof(client_addr);
			if (getpeername(client_fd, (struct sockaddr*)&client_addr, &addr_len) != 0) throw std::runtime_error("cant get ip in Server::task");
			if (!(this->sp_ip_access_control->isAllow(std::make_shared<std::string>(inet_ntoa(client_addr.sin_addr))))) throw httpd::HttpException(StatusCodeAndMessage::Type::Forbidden);
		}
		char buf_in[4096] = {};
		fd_set read_set;
		FD_ZERO(&read_set);
		FD_SET(client_fd, &read_set); //利用select实现read超时
		struct timeval timeout;
		timeout.tv_sec = READ_TIMEOUT_SEC;
		timeout.tv_usec = 0;
		while(1){
			try{
				int select_result = select(client_fd + 1, &read_set, NULL, NULL, &timeout); //在timeout时间内监听是否可以read
				if (-1==select_result) httpd::HttpException(httpd::StatusCodeAndMessage::Type::InternalServerError); //select出错了
				if (0==select_result) throw std::runtime_error("timeout in Server::task"); //超时了
				if (0==read(client_fd,buf_in,sizeof(buf_in))) throw std::runtime_error("disconnect in Server::task"); //客户端断开连接了

				auto request=std::make_shared<httpd::Request>();
				request->decode(std::make_shared<std::string>(buf_in));
				if (nullptr==request->getHeader(std::make_shared<std::string>("Host"))) throw httpd::HttpException(httpd::StatusCodeAndMessage::Type::BadRequest); //请求头中没有Host字段

				if (nullptr==this->message_callback) httpd::HttpException(httpd::StatusCodeAndMessage::Type::InternalServerError);
				auto response=(this->message_callback)(request); //调用消息处理回调

				response->setHeader(std::make_shared<std::string>("Server"),std::make_shared<std::string>("USER202334261359"));
				auto str=response->encode();
				write(client_fd,str->c_str(),str->length());

				auto sp_close=request->getHeader(std::make_shared<std::string>("connection"));
				if (nullptr!=sp_close && *sp_close=="close") throw std::runtime_error("disconnect in Server::task");
			}
			catch(const httpd::HttpException& e){
				std::cerr << e.what() << '\n';
				auto response=Response::quickBuild(e.getStatusCodeAndMessage());
				response->setHeader(std::make_shared<std::string>("Server"),std::make_shared<std::string>("USER202334261359"));
				auto str=response->encode();
				write(client_fd,str->c_str(),str->length());
			}
			catch(const std::exception& e){
				std::cerr << e.what() << '\n';
				close(client_fd);
				break;
			}
		}
	}
	catch(const httpd::HttpException& e){
		std::cerr << e.what() << '\n';
		auto response=Response::quickBuild(e.getStatusCodeAndMessage());
		response->setHeader(std::make_shared<std::string>("Server"),std::make_shared<std::string>("USER202334261359"));
		auto str=response->encode();
		write(client_fd,str->c_str(),str->length());
		close(client_fd);
	}
	catch(const std::exception& e){
		std::cerr << e.what() << '\n';
		close(client_fd);
	}
	
}



} // namespace httpd


//消息处理回调
std::shared_ptr<httpd::Response> onMessage(const std::shared_ptr<httpd::Request> request, const std::shared_ptr<std::string> doc_root){
	std::cout<<*(request->getPath())<<std::endl;

    if (*(request->getMethod())==httpd::Method::Type::POST) throw httpd::HttpException(httpd::StatusCodeAndMessage::Type::InternalServerError); //暂时不能处理POST方法

	httpd::FileSystem fs(doc_root);
	if ("/"==*(request->getPath())) request->setPath(std::make_shared<std::string>("/index.html")); //将/路径设置为/index.html
    auto body=fs.read(request->getPath());

    auto response=std::make_shared<httpd::Response>();
    response->setStatusCodeAndMessage(std::make_shared<httpd::StatusCodeAndMessage>(httpd::StatusCodeAndMessage::Type::OK));
    response->setBody(body);

    return response;
}

void start_httpd(unsigned short port, std::string doc_root, size_t pool_size){
	std::cerr << "Starting server (port: " << port <<
		", doc_root: " << doc_root << ")" << std::endl;
	
    httpd::Server server(port,pool_size,std::make_shared<std::string>("./"+doc_root+"/.htaccess"));
    server.setMessageCallback(std::bind(onMessage,std::placeholders::_1,std::make_shared<decltype(doc_root)>(doc_root)));
    server.run();
}
