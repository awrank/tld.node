#include <stdlib.h>
#include <stdio.h>

/////////////////////////////////////////////////////
//	Module TLD (BETA)
/////////////////////////////////////////////////////
namespace module_tld
{
	const unsigned char POINT 		= 0x2E;		//'.'
	const unsigned char SLASH		= 0x2F;		//'/'
	const unsigned char EOL 		= 0x0A;		//'\n'
	const unsigned char ZERO		= 0x00;		//'\0'
	const unsigned char ATCOM 		= 0x40;		//'@'
	const unsigned char DPOINT		= 0x3A;		//':'
	const unsigned char WILDCARD	= 0x2A;		//'*'
	const unsigned char EXCEPTION	= 0x21;		//'!'
	
	const int FIND_MODE_EXACT 		= 1;
	const int FIND_MODE_STD			= 2;
	const int FIND_MODE_TEMPL		= 3;

	const int INVALID 	= -1;
	const int BYTE		= 8;

	const unsigned char MASK_2		= 0xC0;		//11000000
	const unsigned char MASK_3  	= 0x20;		//00100000
	const unsigned char MASK_4		= 0x10;		//00010000
	const unsigned char MASK_FULL	= 0xFF;		//11111111

	const int SUCCESS		= 0;	//TLD found
	const int RESERVED 		= 1;	//TLD is reserved (deprecated, etc.)
	const int ILLEGAL 		= 2;	//TLD has no point
	const int BADURI 		= 3;	//TLD has '..' (two poins)
	const int NOTFOUND		= 4;	//TLD is unknown
	
	//
	// node of dynamical tree
	//
	typedef struct node
	{
		unsigned int value;
		struct node* next;
		struct node* link;

		struct node* init(unsigned int v=0, node* l=0, node* n=0)
		{
			value = v;
			link = l;
			next = n;
			return this;
		}

		node(unsigned int v=0, node* l=0, node* n=0)
		{
			init(v,l,n);
		}
	} node;

	//
	// tld base
	//
	typedef struct root
	{
		node* base;
		node* reserved;
		node* templates;
		
		struct root* init(node* b=0, node* r=0, node* t=0)
		{
			base = b;
			reserved = r;
			templates = t;
			return this;
		}
		
		root(node* b=0, node* r=0, node* t=0)
		{
			init(b,r,t);
		}
	} root;
	
	//
	// Align utf-8 symbols by 4-bytes sequences
	//
	unsigned int get_symbol(const unsigned char* src, unsigned int& index)
	{
		unsigned int symbol = src[index] & MASK_FULL;
		
		int offset = 1;
		if((src[index] & MASK_2) == MASK_2) //11xxxxxx
		{
			symbol <<= BYTE;
			symbol |= src[index + offset++] & MASK_FULL;
			
			if(src[index] & MASK_3)       //111xxxxx
			{
				symbol <<= BYTE;
				symbol |= src[index + offset++] & MASK_FULL;
				
				if(src[index] & MASK_4)   //1111xxxx
				{
					symbol <<= BYTE;
					symbol |= src[index + offset++] & MASK_FULL;
				}
			}
		}
		
		index += offset;
		
		return symbol;
	}
	void int2utf(unsigned char* dest, unsigned int dest_size, const unsigned int* src, unsigned int src_size)
	{
		unsigned int index = 0;
		unsigned int column = 0;
		while(index<src_size && src[index] && column<dest_size)
		{
			unsigned char* p = (unsigned char*)&src[index++];
			for(int i=sizeof(int)-1; i>=0; i--)
			{
				if(p[i] & MASK_FULL)
				{
					dest[column++] = p[i];
				}
			}
		}
		if(column==dest_size) column--;
		dest[column] = ZERO;
	}
	unsigned int utf2int(unsigned int* dest, unsigned int dest_size, const unsigned char* src, unsigned int src_size)
	{
		unsigned int column = 0;
		unsigned int index = 0;
		while(src[index] && index<src_size && column<dest_size)
		{
			dest[column++] = get_symbol(src, index);
		}
		dest[column] = ZERO;
		
		return column;
	}
	
	//
	// Add word (domain) in the tree
	//
	void add_template(const node* tree, const unsigned int* word, unsigned int size)
	{
		node* p_node = (node*)tree;
		while(p_node->next)
		{
			p_node = p_node->next;
		}
		p_node->next = (node*)malloc(sizeof(node));
		p_node = p_node->next;
		p_node->init(word[size-1]);
		for(unsigned int index=1; index<= size; index++)
		{
			unsigned int symbol = index==size ? POINT : word[size-index-1];
			
			p_node->link = (node*)malloc(sizeof(node));
			p_node = p_node->link;
			p_node->init(symbol);
		}
	}
	void add_word(const node* tree, const unsigned int* word, unsigned int size)
	{
		node* p_node = (node*)tree;
		for(unsigned int index=0; index <= size; index++)
		{
			unsigned int symbol = index==size ? POINT : word[size-index-1];
			
			if(!p_node->link)
			{
				p_node->link = (node*)malloc(sizeof(node));
				p_node->link->init(symbol);
				
				p_node = p_node->link;
			}
			else
			{
				p_node = p_node->link;
				while(p_node->next && p_node->value != symbol)
				{
					p_node = p_node->next;
				}
				
				if(!p_node->next && p_node->value != symbol)
				{
					p_node->next = (node*)malloc(sizeof(node));
					p_node->next->init(symbol);
					
					p_node = p_node->next;
				}
			}
		}
	}

	//
	// Drop word (domain) if exists
	//
	int drop_char(node* prev_tree, const node* tree, const unsigned int* word, int index)
	{
		node* p_node = (node*)tree;
		node* p_prev_node = prev_tree;
		unsigned int value = index<0 ? POINT : word[index];
		
		while(p_node && p_node->value != value)
		{
			p_prev_node = p_node;
			p_node = p_node->next;
		}
		if(!p_node)
		{
			return 0;
		}
		
		if(!p_node->link || drop_char(p_node, p_node->link, word, --index))
		{
			int result = 0;
			if(p_prev_node)
			{
				if(p_prev_node == prev_tree)
				{
					if(!p_node->next)
					{
						result = 1;
					}
					p_prev_node->link = p_node->next;
				}
				else
				{
					p_prev_node->next = p_node->next;
				}
				
				free(p_node);
			}
			return result;
		}
		
		return 0;
	}
	int drop_word(const node* tree, const unsigned int* word, unsigned int size)
	{
		return drop_char(NULL, tree, word, size);
	}

	//
	// Parse input stream (buffer)
	//
	void read_comment(const unsigned char* src, unsigned int src_size, unsigned int& index)
	{
		while(src[index] == SLASH)
		{
			while(index < src_size && src[index] && src[index] != EOL)
			{
				index++;
			};
		}
	}
	void read_line(unsigned int* dest, unsigned int dest_size, const unsigned char* src, unsigned int src_size, unsigned int& index, unsigned int& column)
	{
		while(index < src_size && column<dest_size && src[index] && src[index] != EOL)
		{
			dest[column++] = get_symbol(src, index);
		}
	}
	
	//
	// Clear tld-base
	//
	void clear_tree(node* tree)
	{
		if(tree->next)
		{
			clear_tree(tree->next);
			free(tree->next);
			tree->next = 0;
		}
		
		if(tree->link)
		{
			clear_tree(tree->link);
			free(tree->link);
			tree->link = 0;
		}
	}

	//
	// Parse URL
	//
	unsigned int select_dn(unsigned int* dest, unsigned int dest_size, const unsigned char* src, unsigned int src_size)
	{
		unsigned int index = 0;
		while(index < src_size && src[index] && src[index]!= SLASH && src[index]!=EOL)	//first '/'
		{
			index++;
		}
		
		unsigned int sl_index = index;
		index = 0;
		while(index < src_size && src[index] && src[index]!=ATCOM)	//first '@'
		{
			index++;
		}
		
		int at_index = INVALID;
		if(index < src_size && src[index])
		{
			at_index = index;
		}
		
		index = at_index+1;
		while(index < src_size && src[index] && src[index]!=DPOINT)	//':' after '@'
		{
			index++;
		}
		
		int port_index = sl_index;
		if(index < sl_index && src[index])
		{
			port_index = index;
		}
		
		dest[0] = POINT;											//'.' prefix
		return utf2int(dest+1, dest_size-1, src+at_index+1, port_index-at_index-1)+1;
	}
	
	//
	// Perform URL
	//
	int check_node(const node* p_node, unsigned int symbol, int mode)
	{
		int result = (symbol == POINT);
		if(mode == FIND_MODE_EXACT)
		{
			result = result && !p_node->link;
		}
		return result;
	}
	int check_domain(const unsigned int* src, unsigned int src_size)
	{
		int result = SUCCESS;
		
		unsigned int index = 1;
		int has_point = 0;
		unsigned int prev_symbol = ZERO;
		while(index<src_size && src[index])
		{
			unsigned int symbol = src[index];
			if(symbol == POINT)
			{
				if(prev_symbol == POINT)
				{
					return BADURI;
				}
				has_point = 1;
			}
			prev_symbol = symbol;
			index++;
		}
		if(!has_point)
		{
			result = ILLEGAL;
		}
		return result;
	}
	int find_word(const unsigned int* src, unsigned int src_size, const node* tree, int mode, int& exc)
	{
		node* p_node = (node*)tree;
		exc = 0;
		int index = src_size;
		int tld_index = INVALID;
		while(index >= 0 && p_node)
		{
			unsigned int symbol = src[index];
			
			if(p_node->value == symbol)
			{
				if(check_node(p_node, symbol, mode))
				{
					tld_index = index;
				}
				
				index--;
				p_node = p_node->link;
			}
			else
			{
				if(symbol != POINT)
				{
					p_node = p_node->next;
					while(p_node && p_node->value != symbol)
					{
						p_node = p_node->next;
					}
				}
				else
				{
					while(p_node && p_node->value!=POINT && p_node->value!=EXCEPTION)
					{
						p_node = p_node->next;
					}
				}
				
				if(!p_node)
				{
					return tld_index;
				}
				if(p_node->value==EXCEPTION && symbol==POINT)
				{
					exc=1;
					return tld_index;
				}
				
				if(check_node(p_node, symbol, mode))
				{
					tld_index = index;
				}
				
				index--;
				p_node = p_node->link;
			}
		}
		return tld_index;
	}
	int find_tld(const unsigned int* src, unsigned int src_size, const root* tree, int& exc)
	{
		return find_word(src, src_size, tree->base, FIND_MODE_STD, exc);
	}
	int find_reserved(const unsigned int* src, unsigned int src_size, const root* tree)
	{
		int exc = 0;
		return find_word(src, src_size, tree->reserved, FIND_MODE_EXACT, exc);
	}
	int find_template(const unsigned int* src, unsigned int src_size, const root* tree)
	{
		int result_index = INVALID;
		node* p_start = tree->templates->next;
		while(p_start)
		{			
			if(p_start->value == src[src_size-1])
			{
				node* p_node = p_start->link;
				int index = src_size - 2;
				int tld_index = INVALID;
				
				while(index>=0 && p_node && (p_node->value==src[index] || (p_node->value==WILDCARD && src[index]!=POINT)))
				{
					if(src[index] == POINT)
					{
						tld_index = index;
					}
					
					index--;
					if(p_node->value != WILDCARD || src[index]==POINT)
					{
						p_node = p_node->link;
					}
				}
				
				//printf("%d\n", tld_index);
				if(tld_index!=INVALID && (result_index==INVALID || tld_index<result_index))
				{
					result_index = tld_index;
				}
			}
			
			p_start = p_start->next;
		}
		return result_index;
	}
	int split_domains(const unsigned int* src, unsigned int src_size, unsigned int* dest, unsigned int dest_size)
	{
		unsigned count = 0;
		unsigned int index = 0;
		while(index<src_size && src[index] && count<dest_size)
		{
			if(src[index] == POINT)
			{
				dest[count++] = index;
			}
			index++;
		}
		dest[count] = index;
		return count;
	}
}