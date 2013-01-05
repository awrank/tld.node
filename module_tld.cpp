#include <stdlib.h>
#include <stdio.h>

/////////////////////////////////////////////////////
//	Module TLD (ALPHA)
/////////////////////////////////////////////////////
namespace module_tld
{
	const unsigned char POINT 		= 0x2E;		//'.'
	const unsigned char SLASH		= 0x2F;		//'/'
	const unsigned char EOL 		= 0x0A;		//'\n'
	const unsigned char ZERO		= 0x00;		//'\0'
	const unsigned char ATCOM 		= 0x40;		//'@'
	const unsigned char DPOINT		= 0x3A;		//':'

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
	// node of dynamocal tree
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
	// statistic of tree
	//
	typedef struct stat
	{
		int nodes;			//tree statistic
		int memory;
		
		int lines;			//input stream statistic (for create tree())
		int domains;
		int symbols;

		struct stat* init(int n=0, int m=0, int d=0, int l=0, int s=0)
		{
			nodes = n;
			memory = m;
			domains = d;
			lines = l;
			symbols = s;
			return this;
		}
		
		stat(int n=0, int m=0, int d=0, int l=0, int s=0)
		{
			init(n,m,d,l,s);
		}
	} stat;

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
	// Add word (doamin) in the tree
	//
	void add_word(const node* tree, const unsigned int* word, unsigned int size, stat& s)
	{
		node* p = (node*)tree;
		for(unsigned int index=0; index <= size; index++)
		{
			unsigned int value = index==size ? POINT : word[size-index-1];
			
			if(!p->link)
			{
				p->link = (node*)malloc(sizeof(node));
				p->link->init(value);
				
				s.memory += sizeof(node);
				s.nodes++;
				p = p->link;
			}
			else
			{
				p = p->link;
				while(p->next && p->value != value)
				{
					p = p->next;
				}
				
				if(!p->next && p->value != value)
				{
					p->next = (node*)malloc(sizeof(node));
					p->next->init(value);
					
					s.memory += sizeof(node);
					s.nodes++;
					p = p->next;
				}
			}
		}
	}

	//
	// Drop word (domain) if exists
	//
	int drop_char(node* prev_tree, const node* tree, const unsigned int* word, int index, stat& s)
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
		
		if(!p_node->link || drop_char(p_node, p_node->link, word, --index, s))
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
				s.nodes--;
				s.memory-=sizeof(node);
				free(p_node);
			}
			return result;
		}
		
		return 0;
	}
	int drop_word(const node* tree, const unsigned int* word, unsigned int size, stat& s)
	{
		return drop_char(NULL, tree, word, size, s);
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
	// Create tld-base from utf-8 stream (buffer)
	//
	void create_tree(const node* tree, unsigned int* dest, unsigned int dest_size, const unsigned char* src, unsigned int src_size, stat& s)
	{
		unsigned int index = 0;
		s.init(1,sizeof(node),0,s.lines,0);	//first node contains '\0'
		
		while(index < src_size && src[index])
		{
			read_comment(src, src_size, index);
			
			unsigned int column = 0;
			read_line(dest, dest_size, src, src_size, index, column);
			
			if(column > 0)
			{
				if(src[index] == EOL ||    //EOL
				   src[index] == ZERO)     //EOF
				{
					dest[column] = 0;
					add_word(tree, dest, column, s);
					
					s.domains++;
					s.symbols+=column;
				}
			}
			
			index++;
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
	int check_tld(const unsigned int* src, unsigned int src_size)
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
	int find_tld(const unsigned int* src, unsigned int src_size, const node* tree, int exact=0)
	{
		node* p_node = (node*)tree;
		int symbol_index = src_size;
		int tld_index = INVALID;
		while(symbol_index >= 0 && p_node)
		{
			unsigned int symbol = src[symbol_index];
			if(symbol == p_node->value)
			{
				if(symbol == POINT && (!exact || !p_node->link))
				{
					tld_index = symbol_index;
				}
				
				symbol_index--;
				p_node = p_node->link;
			}
			else
			{
				p_node = p_node->next;
				while(p_node && p_node->value != symbol)
				{
					p_node = p_node->next;
				}
				
				if(!p_node)
				{
					return tld_index;
				}
				else
				{
					if(symbol == POINT && (!exact || !p_node->link))
					{
						tld_index = symbol_index;
					}
					
					symbol_index--;
					p_node = p_node->link;
				}
			}
		}
		return tld_index;
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