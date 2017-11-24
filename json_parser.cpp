#include <yajl/yajl_parse.h>
#include <yajl/yajl_gen.h>

#include <cstdlib>
#include <cstring>
#include <string>
#include <iostream>
#include <memory>

enum type_json 
{
	JObject,
	JArray,
	JString,
	JInteger,
	JKeyValue,
	JUndefined
};

/*
class Product {
public:
	Product() {}
	virtual ~Product() {}
	virtual type_json type(){};
	virtual Product* get() {}; 
	virtual Product* get(const char*){};
	virtual Product* get(int){};
	Product* next;
	Product* prev;
	virtual Product* set(const unsigned char*, unsigned int){};
	virtual Product* set(int){};
	virtual Product* set(Product*){};
	virtual void new_obj(type_json);
};

class KeyValue_JSON: public Product {
public:
	unsigned char* key;
	Product* value;
	KeyValue_JSON()
	{
		key = NULL;
		value = NULL;
		next = NULL;
		prev = NULL;
	}
	~KeyValue_JSON()
	{
		if(this->key)std::cout << "free key:" << this->key << " - " << (int)this->key << std::endl;
		else std::cout << "free key: void" << std::endl;
		if(this->value)std::cout << "free value:" << (int)this->value << std::endl;
		else std::cout << "free value: void" << std::endl;
		if(this->key) delete[] this->key;
		if(this->value) delete this->value;
	}
	type_json type() { return JKeyValue; }
	Product* get() { return value; }
	Product* get(const char*) { return NULL; };
	Product* get(int) { return NULL; };

	Product* set(const unsigned char* k, unsigned int len)
	{
		key = new unsigned char[len];// - (sizeof(const unsigned))];
		//strcpy(key, k);
		memcpy(key, k, len);
		key[len] = 0;
		std::cout << "set key:" << key << std::endl;
		return NULL; 
	}
	Product* set(int tt) { return NULL; };
	Product* set(Product** v)
	{
		value = *v;
	}
	//void new_obj(type_json);
};

class Object_JSON:   public Product {
public:
	type_json type() { return JObject; }
	Product* kv;
	Product* first_kv;
	Object_JSON()
	{
		first_kv = new KeyValue_JSON();
		kv = first_kv;
	}
	~Object_JSON()
	{
		Product* tmp;
		kv = first_kv;
		while(kv)
		{
			std::cout << " ======================== free KV:" << (int)this->kv << " ============================="<< std::endl;
			tmp = kv->next;
			delete kv;
			std::cout << " =========================================================================================" << std::endl;
			kv = tmp;
		}
	}
	Product* get(){	return first_kv; }
	Product* get(const char* k)
	{
		
		kv = first_kv;
		while(kv)
		{
			if( ((KeyValue_JSON*)kv)->key )
				if(strcmp(((KeyValue_JSON*)kv)->key, k) == 0) return ((KeyValue_JSON*)kv)->value;
			kv = kv->next;
		}
		
		return NULL;
	}
	Product* get(int index){ return NULL; }

	Product* set(const unsigned char* k, unsigned int len)
	{
		return kv->set(k, len);
	}
	Product* set(int sig)
	{ 
		if(sig == 0) return first_kv->prev;
		if(sig == 1)
		{
			kv->prev->get()->get()->prev = this;
			return kv->prev->get();
		}
		return NULL;
	}
	Product* set(Product* v)
	{
		kv->set(v);
		kv->next = new KeyValue_JSON();
		kv->next->prev = kv;
		kv = kv->next;
		return NULL; 
	}
};

class Array_JSON:    public Product {
public:
	type_json type() { return JArray; }
	int count;
	Product* kv;
	Product* first_kv;
	Array_JSON() {
		kv = new KeyValue_JSON();
		first_kv = kv;
		count = 0;
	}
	~Array_JSON()
	{
		Product* tmp;
		kv = first_kv;
		while(kv)
		{
			tmp = kv->next;
			delete kv;
			kv = tmp;
		}
	}
	Product* get(){	return first_kv; }
	Product* get(const char* k){ return NULL; }
	Product* get(int index)
	{ 
		kv = first_kv;
		for(int i = 0; i <= count; i++)
		{
			if(i == index) return kv->get();
			if(kv->next) kv = kv->next;
			else return NULL;
		}
		return NULL;
	}

	Product* set(const unsigned char*, unsigned int) { return NULL; };
	Product* set(int sig)
	{
		if(sig == 0) return first_kv->prev;
		if(sig == 1)
		{
			kv->prev->get()->get()->prev = this;
			return kv->prev->get();
		}
		return NULL;
	}
	Product* set(Product* v)
	{
		kv->set(v);
		kv->next = new KeyValue_JSON();
		kv->next->prev = kv;
		kv = kv->next;
		count++;
		return NULL; 
	}
};

class String_JSON:   public Product {
public:
	type_json type() { return JString; }
	unsigned char* str_val;
	~String_JSON()
	{
		std::cout << "free str: " << (int)this->str_val << std::endl;
		
		if(this->str_val) delete[] this->str_val;
		this->str_val = NULL;
		
		std::cout << "is free: " << (int)this->str_val << std::endl;
	}
	String_JSON() {}
	String_JSON(const unsigned char* v, unsigned int len)
	{
		str_val = new unsigned char[len];
		//strcpy(str_val, v);
		memcpy(str_val, v, len);
		str_val[len] = 0;
	}
	Product* get(){ return NULL; }
	Product* get(const char* k){  return NULL; }
	Product* get(int index){  return NULL; }
	
	Product* set(const unsigned char* v, unsigned int len)
	{
		str_val = new unsigned char[len];
		//strcpy(str_val, v);
		memcpy(str_val, v, len);
		return NULL; 
	}
	Product* set(int) { return NULL; };
	Product* set(Product*) { return NULL; };
};

class Integer_JSON:  public Product {
public:
	type_json type() { return JInteger; }
	long value;
	Integer_JSON(const //JSON_int_tlong v)
	{
		value = v;
	}
	Product* get(){	return NULL; }
	Product* get(const char* k){ return NULL; }
	Product* get(int v){ v = value;  return NULL; }
	
	Product* set(const unsigned char* v, unsigned int){ return NULL; }
	Product* set(int v){ value = v;  return NULL; }
	Product* set(Product*){ return NULL; }
};

void
Product::new_obj(type_json tj)
{
	if(tj== JObject)
	{
		this->set(new Object_JSON());
	} 
	else if(tj== JArray)
	{
		this->set(new Array_JSON());
	}
	else if(tj== JString)
	{
		this->set(new String_JSON());
	}
	else if(tj== JInteger)
	{
		this->set(new Integer_JSON(0));
	}
}
*/

struct obj_val {
	const unsigned char* str_val;
	unsigned int str_len;
};


class Object
{
	public:
		Object *next, *prev;
		bool is_openned;
		
		virtual	~Object() {}
		virtual type_json type() { return JUndefined; }
		virtual void* get() { return NULL; }
};

class JSONObject : public Object
{
	public:
		JSONObject() { is_openned = true; }
		type_json type() { return JObject; }
};
class JSONArray : public Object
{
	public:
		JSONArray() { is_openned = true; }
		type_json type() { return JArray; }
};
class JSONString : public Object
{
	public:
		type_json type() { return JString; }
		unsigned char* str_val;
		unsigned int str_len;
		JSONString(const unsigned char* s, unsigned int len)
		{
			str_val = new unsigned char[len+16];
			memcpy(str_val, s, len);
			str_val[len] = 0;
			str_len = len;
		}
		~JSONString()
		{
			delete[] str_val;
		}
		void* get()
		{
			return (void*) str_val;
		}
};
class JSONInteger : public Object
{
	public:
		type_json type() { return JInteger; }
		double d_val;
		virtual void* get() { return (void*)&d_val; }
		JSONInteger(const unsigned char* s, unsigned int len)
		{
			if(len == 0) d_val = 1;
			else
			{
				char* tmp = new char[len+16];
				memcpy(tmp, s, len);
				tmp[len] = 0;
				d_val = atof(tmp);
			}
		}
};
class JSONUndefined : public Object
{
	public:
		type_json type() { return JUndefined; }
};
class JSONKeyValue : public Object
{
	public:
		JSONKeyValue* right, *prev, *left;
		Object* pObj;
		unsigned char* key;
		
		JSONKeyValue()
		{
			right = prev = left = NULL;
			key = NULL;
			pObj = new JSONUndefined();
		}
		~JSONKeyValue()
		{
			if(key) delete[] key;
			if(pObj) delete pObj;
			
			if(right) delete right;
			if(left) delete left;
		}
		
		Object* find_key(char* kz, int lenz)
		{ 
			JSONKeyValue* finder = this;
			while(finder)
			{
				if(finder->key)
				{
					if(memcmp(finder->key, kz, lenz) == 0)
					{
						return finder->pObj;
					}
				}
				finder = finder->right;
			}
			return NULL;
		}
		
		void set_key(const unsigned char* k, unsigned int len)
		{
			key = new unsigned char[len+16];
			memcpy(key, k, len);
			key[len] = 0;
		}
		
		void set_type(type_json tj, obj_val &ov)
		{
			if(tj == JObject)
			{
				delete pObj;
				pObj = new JSONObject();
			}
			else if(tj == JArray)
			{
				delete pObj;
				pObj = new JSONArray();
			}
			else if(tj == JString)
			{
				delete pObj;
				pObj = new JSONString(ov.str_val, ov.str_len);
			}
			else if(tj == JInteger)
			{
				delete pObj;
				pObj = new JSONInteger(ov.str_val, ov.str_len);
			}
		} 
};

class JSONCreator 
{
	public:
		JSONKeyValue* currentKV;
		JSONKeyValue* firstKV;
		
		JSONCreator()
		{
			//firstKV = new JSONKeyValue();
			//currentKV = firstKV;
		}
		~JSONCreator()
		{
			//delete firstKV;
		}
		
		void create(type_json tj, obj_val &ov)
		{
			currentKV->right = new JSONKeyValue();
			currentKV->right->prev = currentKV;
			
			currentKV->set_type(tj, ov);
			
			if(tj == JObject || tj == JArray)
			{
				currentKV->left  = new JSONKeyValue();
				currentKV->left->prev = currentKV;
				currentKV = currentKV->left;
			} 
			else
			{
				currentKV = currentKV->right;
			}
		}
		
		void set_key(const unsigned char* k, unsigned int len)
		{
			currentKV->set_key(k, len);
		}
		
		void back_up()
		{
			while( !(((currentKV->pObj->type() == JObject) || (currentKV->pObj->type() == JArray)) && (currentKV->pObj->is_openned == true)) )
			{
				currentKV = currentKV->prev;
			}
			currentKV->pObj->is_openned = false;
			currentKV = currentKV->right;
		}
};

void print_obj(JSONKeyValue* KV)
{
	if(KV)
	{
		if(KV->pObj->type() != JUndefined)
		{
			if(KV->key) std::cout << "\"" << KV->key << "\": ";
			
			if(KV->pObj->type() == JObject)
			{
				std::cout << "{";
				print_obj(KV->left);
				std::cout << "}";
			}
			else if(KV->pObj->type() == JArray)
			{
				std::cout << "[";
				print_obj(KV->left);
				std::cout << "]";
			}			
			else if(KV->pObj->type() == JString)
			{
				std::cout << "\"" << (char*)KV->pObj->get() << "\"";
			}
			else if(KV->pObj->type() == JInteger)
			{
				std::cout << *((double*)KV->pObj->get());
			}
			
			if(KV->right) if(KV->right->right) std::cout << ", ";
			print_obj(KV->right);
		}
	}
}

/*
void print_obj(JSONCreator* pObj)
{
	if(pObj->type() == JKeyValue)
	{
		if( ((KeyValue_JSON*)pObj)->key )
		{
			std::cout << "\"";
			std::cout << ((KeyValue_JSON*)pObj)->key;
			std::cout << "\": ";
		}
		print_obj( ((KeyValue_JSON*)pObj)->value );		
		if( ((KeyValue_JSON*)pObj->next)->value )
		{
			std::cout << ", ";
			print_obj( pObj->next );
		}
	}
	else if(pObj->type() == JArray)
	{
		std::cout << "[";
		print_obj( pObj->get() );
		std::cout << "]";
	}
	else if(pObj->type() == JObject)
	{
		std::cout << "{";
		print_obj( pObj->get() );
		std::cout << "}";
	}
	else if(pObj->type() == JString)
	{
		std::cout << "\"" << ((String_JSON*)pObj)->str_val << "\"";
	}
	else if(pObj->type() == JInteger)
	{
		std::cout << ((Integer_JSON*)pObj)->value;
	}
}

*/

static int reformat_null(void * _self)
{
	JSONCreator* self = static_cast<JSONCreator*>( _self );
	obj_val ov;
	ov.str_len = 0;
	self->create( JInteger, ov );
    /*yajl_gen g = (yajl_gen) ctx;
    yajl_gen_null(g);*/
    return 1;
}

static int reformat_boolean(void * _self, int boolean)
{
	JSONCreator* self = static_cast<JSONCreator*>( _self );
	obj_val ov;
	ov.str_len = 0;
	self->create( JInteger, ov );
    /*yajl_gen g = (yajl_gen) ctx;
    yajl_gen_bool(g, boolean);*/
    return 1;
}

static int reformat_number(void * _self, const char * s, unsigned int len)
{
	JSONCreator* self = static_cast<JSONCreator*>( _self );
	obj_val ov;
	ov.str_val = (unsigned char*)s;
	ov.str_len = len;
	self->create( JInteger, ov );
    /*yajl_gen g = (yajl_gen) ctx;
    yajl_gen_number(g, s, l);*/
    return 1;
}

static int reformat_string(void * _self, const unsigned char * stringVal,
                           unsigned int stringLen)
{
	JSONCreator* self = static_cast<JSONCreator*>( _self );
	obj_val ov;
	ov.str_val = stringVal;
	ov.str_len = stringLen;
	self->create( JString, ov );
	
/*	Product** self = static_cast<Product**>( _self );
	Product* tzt = new String_JSON(stringVal, stringLen); 
	(*self)->set( (Product*)&tzt );*/
	/*yajl_gen g = (yajl_gen) ctx;
    yajl_gen_string(g, stringVal, stringLen);*/
    return 1;
}

static int reformat_map_key(void * _self, const unsigned char * stringVal,
                            unsigned int stringLen)
{
	JSONCreator* self = static_cast<JSONCreator*>( _self );
	self->set_key( stringVal, stringLen );

/*	Product** self = static_cast<Product**>( _self );
	(*self)->set(stringVal, stringLen);*/
	/*yajl_gen g = (yajl_gen) ctx;
    yajl_gen_string(g, stringVal, stringLen);*/
    return 1;
}

static int reformat_start_map(void * _self)
{
	JSONCreator* self = static_cast<JSONCreator*>( _self );
	obj_val ov;
	self->create( JObject, ov );

/*	Product** self = static_cast<Product**>( _self );
	Product* tzt = new Object_JSON();
	(*self)->set( (Product*)&tzt );
	//(*self)->new_obj( JObject );
	(*self) = (*self)->set(1);*/
    /*yajl_gen g = (yajl_gen) ctx;
    yajl_gen_map_open(g);*/
    return 1;
}


static int reformat_end_map(void * _self)
{
	JSONCreator* self = static_cast<JSONCreator*>( _self );
	self->back_up();

/*	Product** self = static_cast<Product**>( _self );
	(*self) = (*self)->set(0);*/
    /*yajl_gen g = (yajl_gen) _self;
    yajl_gen_map_close(g);*/
    return 1;
}

static int reformat_start_array(void * _self)
{
	JSONCreator* self = static_cast<JSONCreator*>( _self );
	obj_val ov;
	self->create( JArray, ov );
	
/*	Product** self = static_cast<Product**>( _self );
	Product* tzt = new Array_JSON();
	(*self)->set((Product*)&tzt);
	//(*self)->new_obj( JArray );
	(*self) = (*self)->set(1);*/
    /*yajl_gen g = (yajl_gen) ctx;
    yajl_gen_array_open(g);*/
    return 1;
}

static int reformat_end_array(void * _self)
{
	JSONCreator* self = static_cast<JSONCreator*>( _self );
	self->back_up();
	
/*	Product** self = static_cast<Product**>( _self );
	(*self) = (*self)->set(0);*/
    /*yajl_gen g = (yajl_gen) ctx;
    yajl_gen_array_close(g);*/
    return 1;
}

static yajl_callbacks callbacks = {
    reformat_null,
    reformat_boolean,
    NULL,
    NULL,
    reformat_number,
    reformat_string,
    reformat_start_map,
    reformat_map_key,
    reformat_end_map,
    reformat_start_array,
    reformat_end_array
};



void json_parse(char* pMBuf, JSONKeyValue* product)
{
	yajl_handle hand;
	//static unsigned char fileData[11500];
	static unsigned char* fileData = (unsigned char*)pMBuf;
	
	JSONCreator jcreator;
	jcreator.firstKV = product;
	jcreator.currentKV = product;
	
	yajl_status stat;
	size_t rd;
	yajl_parser_config cfg = { 1, 1 };

	hand = yajl_alloc(&callbacks, &cfg, NULL, (void*)&jcreator);
	
	rd = strlen(pMBuf);
	
	stat = yajl_parse(hand, (unsigned char*)pMBuf, static_cast<unsigned int>(rd) );
	stat = yajl_parse_complete(hand);
	
	if (stat != yajl_status_ok &&
		stat != yajl_status_insufficient_data)
	{
		unsigned char * str = yajl_get_error(hand, 1, fileData, rd);
		std::cout << std::endl << "---- yajl_error ----: " << str << std::endl;
		yajl_free_error(hand, str);
	} else {
		std::cout << "---- yajl_ok ----" << std::endl;
		print_obj(jcreator.firstKV);
	}
	
    yajl_free(hand);
	//delete pObj;
}
