#include <node.h>
#include <node_buffer.h>
#include <v8.h>

#include "module_tld.cpp"

using namespace v8;
using namespace node;

Handle<Value> load(const Arguments& args)
{
    HandleScope scope;
    Handle<Object> load_result = Object::New();
    //
    // Get data from argumments
    //
    if(args.Length() < 2 || !(args[0]->IsArray() && args[1]->IsArray()) && !(args[0]->IsObject() && args[1]->IsObject()))
    {
        return v8::ThrowException(Exception::Error(String::NewSymbol("Invalid argument for load(effective_tld_names, reserved_tld_names)")));
    }
	//
	// Check internal base (must be empty)
	//
	Handle<Object> base_obj = args.Holder()->GetHiddenValue(String::NewSymbol("base"))->ToObject();
    Handle<Object> guide_obj = args.Holder()->GetHiddenValue(String::NewSymbol("guide"))->ToObject();
	if(Buffer::Length(base_obj) > 0 || Buffer::Length(guide_obj)>0)
	{
		return v8::ThrowException(Exception::Error(String::NewSymbol("Tld-base is already full. Use release() before load()")));
	}
	//
	// Allocate memory
	//
	module_tld::stat* base_stat = (module_tld::stat*)malloc(sizeof(module_tld::stat));    
	module_tld::stat* guide_stat = (module_tld::stat*)malloc(sizeof(module_tld::stat));
	module_tld::node* base_root = (module_tld::node*)malloc(sizeof(module_tld::node));
	module_tld::node* guide_root = (module_tld::node*)malloc(sizeof(module_tld::node));
	unsigned int word_size = 256;
	unsigned int word[256];
	//
	// Initialize structures
	//
	base_root->init();
	guide_root->init();
	base_stat->init();
	guide_stat->init();
	//
	// Create internal tld-base
	//
	if(args[0]->IsArray() && args[1]->IsArray())
	{
		Local<Array> base_array = Array::Cast(*args[0]);
		Local<Array> guide_array = Array::Cast(*args[1]);
		unsigned int base_count = base_array->Length();
		unsigned int guide_count = guide_array->Length();
		for(unsigned int i=0; i<base_count; i++)
		{
			String::Utf8Value domain_obj(base_array->Get(i)->ToString());
			unsigned char* domain = (unsigned char*)*domain_obj;
			unsigned int domain_size = domain_obj.length();
			unsigned int domain_len = module_tld::utf2int(word, word_size, domain, domain_size);
			module_tld::add_word(base_root, word, domain_len, *base_stat);
		}
		for(unsigned int i=0; i<guide_count; i++)
		{
			String::Utf8Value reserved_obj(guide_array->Get(i)->ToString());
			unsigned char* reserved = (unsigned char*)*reserved_obj;
			unsigned int reserved_size = reserved_obj.length();
			unsigned int reserved_len = module_tld::utf2int(word, word_size, reserved, reserved_size);
			module_tld::add_word(guide_root, word, reserved_len, *guide_stat);
		}
	}
	else
	{
		unsigned char* base_buffer = (unsigned char*)Buffer::Data(args[0]->ToObject());
		unsigned int base_buffer_size = Buffer::Length(args[0]->ToObject());
		unsigned char* guide_buffer = (unsigned char*)Buffer::Data(args[1]->ToObject());
		unsigned int guide_buffer_size = Buffer::Length(args[1]->ToObject());
		module_tld::create_tree(base_root, word, word_size, base_buffer, base_buffer_size, *base_stat);
		module_tld::create_tree(guide_root, word, word_size, guide_buffer, guide_buffer_size, *guide_stat);	
	}
    //
    // Save base int the holder
    //
	args.Holder()->SetHiddenValue(String::NewSymbol("base"), Buffer::New((char*)base_root, sizeof(module_tld::node))->handle_);
	args.Holder()->SetHiddenValue(String::NewSymbol("guide"), Buffer::New((char*)guide_root, sizeof(module_tld::node))->handle_);
    args.Holder()->SetHiddenValue(String::NewSymbol("stat"), Buffer::New((char*)base_stat, sizeof(module_tld::stat))->handle_);
    args.Holder()->SetHiddenValue(String::NewSymbol("gstat"), Buffer::New((char*)guide_stat, sizeof(module_tld::stat))->handle_);
    //
    // Release temporary memory
    //
    free(base_root);
    free(base_stat);
    free(guide_stat);
    free(guide_root);
    
    return scope.Close(load_result); 
}

Handle<Value> tld(const Arguments& args)
{
    HandleScope scope;
    Handle<Object> tld_result = Object::New();
    //
    // Get internal tld-base
    //
    Handle<Object> base_obj = args.Holder()->GetHiddenValue(String::NewSymbol("base"))->ToObject();
    Handle<Object> guide_obj = args.Holder()->GetHiddenValue(String::NewSymbol("guide"))->ToObject();
    Handle<Object> stat_obj = args.Holder()->GetHiddenValue(String::NewSymbol("stat"))->ToObject();
    Handle<Object> gstat_obj = args.Holder()->GetHiddenValue(String::NewSymbol("gstat"))->ToObject();
    if(Buffer::Length(stat_obj) != sizeof(module_tld::stat) ||
       Buffer::Length(gstat_obj) != sizeof(module_tld::stat))
    {
        return v8::ThrowException(Exception::Error(String::NewSymbol("Invalid statistic object. Use method load() to initialize tld base")));
    }
	//
	// Check base size
	//
	if(Buffer::Length(base_obj) != (unsigned int)sizeof(module_tld::node) ||
       Buffer::Length(guide_obj) != (unsigned int)sizeof(module_tld::node))
    {
        return v8::ThrowException(Exception::Error(String::NewSymbol("Invalid base. Use method load() to initialize tld base")));
    }
	module_tld::node* base_root = (module_tld::node*)Buffer::Data(base_obj);
	module_tld::node* guide_root = (module_tld::node*)Buffer::Data(guide_obj);
    //
    // Get url from argument
    //
    if(args.Length() < 1)
    {
        return v8::ThrowException(Exception::Error(String::NewSymbol("Invalid argument. Use tld(v8::String url)")));
    }
    String::Utf8Value url_obj(args[0]->ToString());
    unsigned char* url = (unsigned char*)*url_obj;
    unsigned int url_size = url_obj.length();
    Local<Array> domain_array = Array::New();
    Local<String> domain_str = String::NewSymbol("");
    //
    // Allocate temporary memory
    //
    unsigned int word_size = 256;
    unsigned int tld_size = 256;
	unsigned int domain_list_size = 128;
    unsigned char tld[256];
    unsigned char domain[256];
    unsigned int word[256];
    unsigned int domain_list[128];
    //
    // Parse url
    //
    unsigned int dn_size = module_tld::select_dn(word, word_size, url, url_size);
    unsigned int domains_count = 0;
    int status = module_tld::SUCCESS;
	int check = module_tld::check_tld(word, word_size);

	if(check == module_tld::BADURI)
	{
		status = module_tld::BADURI;
	}
	else if(check == module_tld::ILLEGAL)
	{
		status = module_tld::ILLEGAL;
	}
	else
	{
		int res_index = module_tld::find_tld(word, dn_size, guide_root, 1);
        
		if(res_index == module_tld::INVALID)
		{
			int tld_index = module_tld::find_tld(word, dn_size, base_root);
           
			if(tld_index == module_tld::INVALID)
			{
				status = module_tld::NOTFOUND;
			}
			else
			{
				domains_count = module_tld::split_domains(word, tld_index, domain_list, domain_list_size);
				module_tld::int2utf(tld, tld_size, word+tld_index+1, word_size-tld_index-1);
                domain_str = String::New((char*)tld);
                
                domain_array = Array::New(domains_count);
				for(unsigned int i=0; i<domains_count; i++)
				{
					int domain_size = domain_list[i+1] - domain_list[i];
					module_tld::int2utf(domain, tld_size, word+domain_list[i]+1, domain_size-1);
					domain_array->Set(i, String::New((char*)domain));
				}
			}
		}
		else
		{
			status = module_tld::RESERVED;
		}
	}
    // 
    // Initialize result
    //
    tld_result->Set(String::NewSymbol("status"), Int32::New(status));
    tld_result->Set(String::NewSymbol("domain"), domain_str);
    tld_result->Set(String::NewSymbol("sub_domains"), domain_array);
    
    return scope.Close(tld_result);
}

Handle<Value> update(const Arguments& args)
{
	HandleScope scope;
    Handle<Object> tld_result = Object::New();
	//
	// Get internal tld-base
	//
	Handle<Object> base_obj = args.Holder()->GetHiddenValue(String::NewSymbol("base"))->ToObject();
    Handle<Object> guide_obj = args.Holder()->GetHiddenValue(String::NewSymbol("guide"))->ToObject();
	Handle<Object> stat_obj = args.Holder()->GetHiddenValue(String::NewSymbol("stat"))->ToObject();
    Handle<Object> gstat_obj = args.Holder()->GetHiddenValue(String::NewSymbol("gstat"))->ToObject();
	module_tld::node* base_root = (module_tld::node*)Buffer::Data(base_obj);
	module_tld::node* guide_root = (module_tld::node*)Buffer::Data(guide_obj);
	module_tld::stat* base_stat = (module_tld::stat*)Buffer::Data(stat_obj);
    module_tld::stat* guide_stat = (module_tld::stat*)Buffer::Data(gstat_obj);
	if(Buffer::Length(stat_obj) != sizeof(module_tld::stat) ||
       Buffer::Length(gstat_obj) != sizeof(module_tld::stat))
    {
        return v8::ThrowException(Exception::Error(String::NewSymbol("Invalid statistic object. Use method load() to initialize tld base")));
    }
	if(Buffer::Length(base_obj) != (unsigned int)sizeof(module_tld::node) ||
       Buffer::Length(guide_obj) != (unsigned int)sizeof(module_tld::node))
    {
        return v8::ThrowException(Exception::Error(String::NewSymbol("Invalid base. Use method load() to initialize tld base")));
    }
	//
	// Get arguments
	//
	if(args.Length() < 3 || /*!args[0]->IsStringObject() ||*/ !args[1]->IsInt32() || !args[2]->IsInt32())
    {
        return v8::ThrowException(Exception::Error(String::NewSymbol("Ivalid argument. Use update(v8::String domain, v8::Int32 operation, v8:Int32 type)")));
    }
    String::Utf8Value domain_obj(args[0]->ToString());
    unsigned char* domain = (unsigned char*)*domain_obj;
    unsigned int domain_size = domain_obj.length();
	Local<Int32> operation_obj = args[1]->ToInt32();
	unsigned int operation = (unsigned int)operation_obj->Value();
	Local<Int32> type_obj = args[2]->ToInt32();
	unsigned int type = (unsigned int)type_obj->Value();
	
	unsigned int word_size = 256;
	unsigned int word[256];
	
	module_tld::node* target = 0;
	module_tld::stat* starget = 0;
	if(type == 1)
	{
		target = base_root;
		starget = base_stat;
	}
	else if(type == 2)
	{
		target = guide_root;
		starget = guide_stat;
	}
	else
	{
		return v8::ThrowException(Exception::Error(String::NewSymbol("Invalid type. Use 1 - for tld-base, 2 - for reserved base")));
	}
	
	unsigned int word_len = module_tld::utf2int(word, word_size, domain, domain_size);
	
	if(operation == 1)
	{
		module_tld::add_word(target, word, word_len, *starget);
	}
	else if(operation == 2)
	{
		module_tld::drop_word(target, word, word_len, *starget);
	}
	else
	{
		return v8::ThrowException(Exception::Error(String::NewSymbol("Invalid operation. Use 1 - to add domain, 2 - to drop domain")));
	}
	
	return scope.Close(tld_result);
}

Handle<Value> release(const Arguments& args)
{
    HandleScope scope;
    Handle<Object> tld_result = Object::New();
    //
    // Get base from holder
    //
    Handle<Object> base_obj = args.Holder()->GetHiddenValue(String::NewSymbol("base"))->ToObject();
    Handle<Object> guide_obj = args.Holder()->GetHiddenValue(String::NewSymbol("guide"))->ToObject();	
	module_tld::node* base_root = (module_tld::node*)Buffer::Data(base_obj);
	module_tld::node* guide_root = (module_tld::node*)Buffer::Data(guide_obj);
	//
	// Clear tld-base
	//
    module_tld::clear_tree(base_root);
    module_tld::clear_tree(guide_root);
	//
	// Remove structure from holder
	//
	args.Holder()->SetHiddenValue(String::NewSymbol("base"), Buffer::New(0)->handle_);
    args.Holder()->SetHiddenValue(String::NewSymbol("guide"), Buffer::New(0)->handle_);
	
    return scope.Close(tld_result);
}

void init(Handle<Object> target)
{
    target->SetHiddenValue(String::NewSymbol("base"), Buffer::New(0)->handle_);
    target->SetHiddenValue(String::NewSymbol("guide"), Buffer::New(0)->handle_);
    target->SetHiddenValue(String::NewSymbol("stat"), Buffer::New(0)->handle_);
    target->SetHiddenValue(String::NewSymbol("gstat"), Buffer::New(0)->handle_);
    
    target->Set(String::NewSymbol("load"), FunctionTemplate::New(load)->GetFunction());
    target->Set(String::NewSymbol("tld"), FunctionTemplate::New(tld)->GetFunction());
	target->Set(String::NewSymbol("update"), FunctionTemplate::New(update)->GetFunction());
	target->Set(String::NewSymbol("release"), FunctionTemplate::New(release)->GetFunction());
}

NODE_MODULE(module_tld, init)