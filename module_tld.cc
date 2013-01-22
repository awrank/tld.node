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
    if(args.Length() < 2 || (!(args[0]->IsArray() && args[1]->IsArray()) && !(args[0]->IsObject() && args[1]->IsObject())))
    {
        return v8::ThrowException(Exception::Error(String::NewSymbol("Invalid argument for load(effective_tld_names, reserved_tld_names)")));
    }
	//
	// Check internal base (must be empty)
	//
	Handle<Object> root_obj = args.Holder()->GetHiddenValue(String::NewSymbol("root"))->ToObject();
	if(Buffer::Length(root_obj) > 0)
	{
		return v8::ThrowException(Exception::Error(String::NewSymbol("Tld-base is already full. Use release() before load()")));
	}
	//
	// Allocate memory
	//
	module_tld::root* root = (module_tld::root*)malloc(sizeof(module_tld::root));
	root->base = ((module_tld::node*)malloc(sizeof(module_tld::node)))->init();
	root->reserved = ((module_tld::node*)malloc(sizeof(module_tld::node)))->init();
	root->templates = ((module_tld::node*)malloc(sizeof(module_tld::node)))->init();
	unsigned int word_size = 256;
	unsigned int word[256];
	//
	// Create internal TLD-base
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
			
			if(word[0] == module_tld::WILDCARD)							//base contains *.xx
			{
				module_tld::add_template(root->templates, word, domain_len);
			}
			//else if(word[0] == module_tld::EXCEPTION)						//base contains !yy.xx
			//{
			//	module_tld::add_word(root->reserved, word+1, domain_len-1);
			//}
			else
			{
				module_tld::add_word(root->base, word, domain_len);
			}
		}
		for(unsigned int i=0; i<guide_count; i++)
		{
			String::Utf8Value reserved_obj(guide_array->Get(i)->ToString());
			unsigned char* reserved = (unsigned char*)*reserved_obj;
			unsigned int reserved_size = reserved_obj.length();
			unsigned int reserved_len = module_tld::utf2int(word, word_size, reserved, reserved_size);
			
			module_tld::add_word(root->reserved, word, reserved_len);
		}
	}
	else
	{
		unsigned char* base_buffer = (unsigned char*)Buffer::Data(args[0]->ToObject());
		unsigned int base_buffer_size = Buffer::Length(args[0]->ToObject());
		unsigned char* guide_buffer = (unsigned char*)Buffer::Data(args[1]->ToObject());
		unsigned int guide_buffer_size = Buffer::Length(args[1]->ToObject());
		//
		// Process first argument (base can contains TLD, wildcards and exceptions)
		//
		unsigned int index = 0;
		while(index < base_buffer_size && base_buffer[index])
		{
			module_tld::read_comment(base_buffer, base_buffer_size, index);
			
			unsigned int column = 0;
			module_tld::read_line(word, word_size, base_buffer, base_buffer_size, index, column);
			
			if(column > 0)
			{
				if(base_buffer[index] == module_tld::EOL || base_buffer[index] == module_tld::ZERO)
				{
					word[column] = 0;
					
					if(word[0] == module_tld::WILDCARD)							//base contains *.xx
					{
						module_tld::add_template(root->templates, word, column);
					}
					//else if(word[0] == module_tld::EXCEPTION)						//base contains !yy.xx
					//{
					//	module_tld::add_word(root->reserved, word+1, column-1);
					//}
					else			
					{
						module_tld::add_word(root->base, word, column);
					}
				}
			}
			index++;
		}
		//
		// Process second argument (add in guide of reserved TLD)
		//
		index = 0;
		while(index < guide_buffer_size && guide_buffer[index])
		{
			module_tld::read_comment(guide_buffer, guide_buffer_size, index);
			
			unsigned int column = 0;
			module_tld::read_line(word, word_size, guide_buffer, guide_buffer_size, index, column);
			
			if(column > 0)
			{
				if(guide_buffer[index] == module_tld::EOL || guide_buffer[index] == module_tld::ZERO)
				{
					word[column] = 0;
					
					module_tld::add_word(root->reserved, word, column);
				}
			}
			index++;
		}
	}
    //
    // Save base int the holder, release temporary memory
    //
	args.Holder()->SetHiddenValue(String::NewSymbol("root"), Buffer::New((char*)root, sizeof(module_tld::root))->handle_);
    free(root);
    
    return scope.Close(load_result); 
}

Handle<Value> tld(const Arguments& args)
{
    HandleScope scope;
    Handle<Object> tld_result = Object::New();
    //
    // Get internal tld-base
    //
    Handle<Object> root_obj = args.Holder()->GetHiddenValue(String::NewSymbol("root"))->ToObject();
    if(Buffer::Length(root_obj) != sizeof(module_tld::root))
    {
        return v8::ThrowException(Exception::Error(String::NewSymbol("Invalid statistic object. Use method load() to initialize tld base")));
    }
	module_tld::root* root = (module_tld::root*)Buffer::Data(root_obj);
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
	int check = module_tld::check_domain(word, word_size);

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
		//
		// Try search reserved TLD
		//
		int res_index = module_tld::find_reserved(word, dn_size, root);
        
		if(res_index == module_tld::INVALID)
		{
			//
			// Try search TLD as word
			//
			int exception = 0;
			int tld_index = module_tld::find_tld(word, dn_size, root, exception);
			int templ_tld_index = module_tld::INVALID;
			if(!exception)templ_tld_index =	module_tld::find_template(word, dn_size, root);
			
			if(tld_index == module_tld::INVALID && templ_tld_index == module_tld::INVALID)
			{
				status = module_tld::NOTFOUND;
			}
			else
			{
				if(tld_index == module_tld::INVALID || (templ_tld_index >= 0 && templ_tld_index < tld_index))
				{
					tld_index = templ_tld_index;
				}
				//
				// Process URL when TLD found
				//
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
	Handle<Object> root_obj = args.Holder()->GetHiddenValue(String::NewSymbol("root"))->ToObject();
	module_tld::root* root = (module_tld::root*)Buffer::Data(root_obj);
	if(Buffer::Length(root_obj) != (unsigned int)sizeof(module_tld::root))
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
	if(type == 1)
	{
		target = root->base;
	}
	else if(type == 2)
	{
		target = root->reserved;
	}
	else
	{
		return v8::ThrowException(Exception::Error(String::NewSymbol("Invalid type. Use 1 - for tld-base, 2 - for reserved base")));
	}
	
	unsigned int word_len = module_tld::utf2int(word, word_size, domain, domain_size);
	
	if(operation == 1)
	{
		module_tld::add_word(target, word, word_len);
	}
	else if(operation == 2)
	{
		module_tld::drop_word(target, word, word_len);
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
    Handle<Object> root_obj = args.Holder()->GetHiddenValue(String::NewSymbol("root"))->ToObject();
	module_tld::root* root = (module_tld::root*)Buffer::Data(root_obj);
	//
	// Clear tld-base
	//
    module_tld::clear_tree(root->base);
    module_tld::clear_tree(root->reserved);
	module_tld::clear_tree(root->templates);
	free(root->base);
	free(root->reserved);
	free(root->templates);
	root->init();
	//
	// Remove structure from holder
	//
	args.Holder()->SetHiddenValue(String::NewSymbol("root"), Buffer::New(0)->handle_);
	
    return scope.Close(tld_result);
}

void init(Handle<Object> target)
{
    target->SetHiddenValue(String::NewSymbol("root"), Buffer::New(0)->handle_);
    
    target->Set(String::NewSymbol("load"), FunctionTemplate::New(load)->GetFunction());
    target->Set(String::NewSymbol("tld"), FunctionTemplate::New(tld)->GetFunction());
	target->Set(String::NewSymbol("update"), FunctionTemplate::New(update)->GetFunction());
	target->Set(String::NewSymbol("release"), FunctionTemplate::New(release)->GetFunction());
}

NODE_MODULE(module_tld, init)