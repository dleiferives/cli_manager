#ifndef CLI_INPUT_MANAGER_DLEIFERIVES_H
#define CLI_INPUT_MANAGER_DLEIFERIVES_H
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

struct InputToken{
	char *value;
	struct InputToken *next;
};

struct InputToken *InputToken_init(char * str)
{
	struct InputToken * token = (struct InputToken *)calloc(1,sizeof(struct InputToken));
	token->value = (char *)malloc(sizeof(char) * strlen(str));
	strcpy(token->value,str);
	token->next = NULL;
	return token;
}
typedef struct InputManager InputManager;
typedef struct{
	void*(*exec)(struct InputManager*,struct InputToken*);
	char *flag;
	void *response;
	char *desc_string;
}InputLink;

struct InputManager
{
	int (*init)(struct InputManager *);
	int (*bind)(struct InputManager *,void *(struct InputManager*,struct InputToken *),char*,char*);
	InputLink *input_links;
	struct InputToken* free_tokens_tail;
	struct InputToken free_tokens;
	struct InputToken all_tokens;
	void *(*parse)(struct InputManager*,int,char**);
	int num_links;
	int size_links;
};


void InputManager_append_link(struct InputManager * input_manager, InputLink link)
{
	if(input_manager->num_links+1 == input_manager->size_links)
	{
		InputLink *temp = (InputLink *)realloc(input_manager->input_links, sizeof(InputLink) * 2 * input_manager->size_links);
		if(!temp)
		{
			printf("allocation failed :( for the input manager that is exiting \n");
			exit(0);
		}
		input_manager->input_links = temp;
		input_manager->size_links *= 2;
	}
	input_manager->input_links[input_manager->num_links++] = link;
	return;
}

int CLI_INPUT_MANAGER_bind(struct InputManager *input_manager, void *(*func)(struct InputManager*, struct InputToken *), char *flag, char * desc_string) {
	InputLink link;
	link.exec = func;
	link.flag = (char *)malloc(sizeof(char)*strlen(flag));
	link.response = (void *)-1;
	link.desc_string = (char *)malloc(sizeof(char)*strlen(desc_string));
	strcpy(link.desc_string,desc_string);
	strcpy(link.flag,flag);
	InputManager_append_link(input_manager,link);
	return 1;
}

void * CLI_INPUT_MANAGER_parse(struct InputManager *input_manager, int argc, char * argv[])
{
    	if(argc < 2){
		return NULL;
    	}
	input_manager->all_tokens.value = "start";
	input_manager->all_tokens.next = NULL;
	struct InputToken *token = &input_manager->all_tokens;

	for(int i =1; i <argc; i++)
	{
		token->next = (struct InputToken *)malloc(sizeof(struct InputToken));
		token = token->next;
        	token->value =(char *)malloc(sizeof(char) * strlen(argv[i]));
        	strcpy(token->value,argv[i]);
	}
	token->next=NULL;

	token = input_manager->all_tokens.next;
	for(int i =0; i < input_manager->num_links; i++)
	{
		int token_used =0;
		for(int j=0; j< input_manager->num_links; j++)
		{
			if(!strcmp(input_manager->input_links[j].flag,token->value))
			{
				input_manager->input_links[j].response = input_manager->input_links[j].exec(input_manager,token->next);
				token_used = 1;
			}
		}

		if(!token_used)
		{
			input_manager->free_tokens_tail->next = InputToken_init(token->value);
			input_manager->free_tokens_tail = input_manager->free_tokens_tail->next;
		}
		if(!token->next) break;
		token = token->next;
	}

	return (void *)1;

}

struct InputManager InputManager_create(void)
{
	struct InputManager result;
	result.bind = CLI_INPUT_MANAGER_bind;
	result.input_links = (InputLink *)malloc(sizeof(InputLink) * 10);
	result.size_links = 10;
	result.num_links =0;
	result.init = NULL;
	result.parse = CLI_INPUT_MANAGER_parse;
	result.free_tokens = (struct InputToken){"pass",NULL};
	result.free_tokens_tail = &result.free_tokens;
	return result;
}

struct InputManager InputManager_init(int (*init_function)(struct InputManager *))
{
	struct InputManager result = InputManager_create();
	result.init = init_function;
	result.init(&result);
	return result;
}

void * CLI_INPUT_MANAGER_file_opener(struct InputManager * input_manager, struct InputToken* token)
{
	if(!token)
	{
    		printf("No file found do -file $filename$\n");
    		exit(EXIT_FAILURE);
	}
        FILE* file = fopen(token->value, "r");

        if (file == NULL) {
            // Handle the error here
            perror("Error opening file");
            exit(EXIT_FAILURE); // You may choose to handle the error differently
        }
        return (void*)file;
}

void * CLI_INPUT_MANAGER_help_print_linked_and_flags(struct InputManager * input_manager, struct InputToken * token)
{
   printf("This is the auto generated help menu\n");
   printf("The possible command line arguments are:\n\n");
   for(int i =0; i<input_manager->num_links; i++)
   {
	printf("%s %s\n",input_manager->input_links[i].flag,input_manager->input_links[i].desc_string);
   }
   putchar(10);
   return NULL;
}
       

int CLI_INPUT_MANAGER_init_help_default(struct InputManager * input_manager)
{
	input_manager->bind(input_manager, CLI_INPUT_MANAGER_help_print_linked_and_flags,"-h","this help menu");
	input_manager->bind(input_manager, CLI_INPUT_MANAGER_help_print_linked_and_flags,"-help","this help menu");
	input_manager->bind(input_manager, CLI_INPUT_MANAGER_help_print_linked_and_flags,"--help","this help menu");
	return 1;
}

int CLI_INPUT_MANAGER_init_file_default(struct InputManager * input_manager)
{	
	input_manager->bind(input_manager, CLI_INPUT_MANAGER_file_opener,"-file",
	"opens the next argument in read mode");
	return 1;
}

int CLI_INPUT_MANAGER_init_help_and_file_default(struct InputManager * input_manager)
{
    CLI_INPUT_MANAGER_init_file_default(input_manager);
    CLI_INPUT_MANAGER_init_help_default(input_manager);
    return 1;
}


InputLink InputManager_find_link(struct InputManager * input_manager, char * flag)
{
    for(int i =0; i<input_manager->num_links; i++)
    {
        if(!strcmp(flag,input_manager->input_links[i].flag)){
            return input_manager->input_links[i];
        }
    }
    InputLink i;
    i.response = (void *)-1;
    return i;
}

void InputToken_recursive_free(struct InputToken * token)
{
    if(!token) return;
    
    InputToken_recursive_free(token->next);
    free(token->next);
    token->next = NULL;
    return;
}

void InputManager_free(struct InputManager * input_manager)
{
   for(int i =0; i<input_manager->size_links; i++)
   {
       struct InputToken *temp = input_manager->input_links[i];
       free(temp->flag);
       temp->flag = NULL;
       free(temp->desc_string);
       temp->desc_string = NULL;
   }
   free(input_manager->input_links);
   input_manager->input_links = NULL;
   
   InputToken_recursive_free(input_manager->free_tokens);
   InputToken_recursive_free(input_manager->all_tokens);
   free(input_manager->free_tokens_tail);
   input_manager->free_tokens_tail = NULL;

   free(input_manager);
   input_manager = NULL;
   return;
}

   
   
#define InputManager_get_response(IM, S, TYPE) (TYPE(InputManager_find_link(&IM,S)).response)

#endif

