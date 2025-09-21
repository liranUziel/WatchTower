#include "commands.h"



void tokenizeArgs(int argc, char** argv) {
	int i;
	bool set = false;
	ARG* args = (ARG*)malloc(sizeof(ARG) * (argc - 1));
    if (!args) {
        fprintf(stderr, "Failed to allocate memory for arguments\n");
        return;
    }
	for (i = 1;i < argc;i++) {
		if (!set && argv[i][0] == '-') {
			// Found a flag
			int len = strlen(argv[i]);
			args[i - 1].argValue = (char*)malloc(sizeof(char) * (len));
			if (!args[i - 1].argValue) {
                fprintf(stderr, "Failed to allocate memory\n");
                free(args);
                return;
            }
			memcpy(args[i - 1].argValue, argv[i]+1, len-1);
			args[i - 1].argValue[len] = '\0';
			args[i - 1].type = FLAGS;
			set = true;
		}else if (set && argv[i][0] != '-'){
			 // Found value for the flag
            int len = strlen(argv[i]);
            args[i - 1].argValue = (char*)malloc(sizeof(char) * (len + 1));
            if (!args[i - 1].argValue) {
                fprintf(stderr, "Failed to allocate memory\n");
                free(args);
                return;
            }
            memcpy(args[i - 1].argValue, argv[i], len);
            args[i - 1].argValue[len] = '\0';
            args[i - 1].type = ARGS;
            set = false;
		} else {
            // Error: unpaired flag or value
			// TODO support mono-flags like -p or -u
            fprintf(stderr, "Error: Unpaired argument: %s\n", argv[i]);
            free(args);
            return;
        }
		if (set) {
			fprintf(stderr, "Error: Flag without value\n");
			free(args);
			return;
		}
	}
	// return value to use in parse
}

CONFIG parseArgs(int argc, char* argv[]) {
	CONFIG config = {0};
	int i;
	bool set = false;
	int flag_count = 0;
	
	// First pass: count how many flags we have
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			flag_count++;
		}
	}
	
	if (flag_count == 0) {
		return config; // No flags found
	}
	
	// Allocate memory for flags and values
	config.flags = (char*)malloc(flag_count + 1);
	config.values = (char**)malloc(sizeof(char*) * flag_count);
	config.flag_count = flag_count;
	
	if (!config.flags || !config.values) {
		fprintf(stderr, "Failed to allocate memory for config\n");
		free(config.flags);
		free(config.values);
		config.flags = NULL;
		config.values = NULL;
		config.flag_count = 0;
		return config;
	}
	
	// Initialize values array
	for (i = 0; i < flag_count; i++) {
		config.values[i] = NULL;
	}
	
	// Second pass: extract flags and values
	int current_flag = 0;
	for (i = 1; i < argc; i++) {
		if (!set && argv[i][0] == '-') {
			// Found a flag
			char flag_char = argv[i][1];
			config.flags[current_flag] = flag_char;
			
			// Check if this flag needs a value (w and C need values, S doesn't)
			if (flag_char == 'w' || flag_char == 'C') {
				set = true; // Expect a value next
			} else if (flag_char == 'S') {
				// Silent mode - no value needed
				config.values[current_flag] = NULL;
				current_flag++;
				set = false;
			} else {
				fprintf(stderr, "Error: Unknown flag: -%c\n", flag_char);
				freeConfig(&config);
				config.flags = NULL;
				return config;
			}
		} else if (set && argv[i][0] != '-') {
			// Found value for the flag
			int len = strlen(argv[i]);
			config.values[current_flag] = (char*)malloc(len + 1);
			if (config.values[current_flag]) {
				strcpy(config.values[current_flag], argv[i]);
			}
			current_flag++;
			set = false;
		} else {
			fprintf(stderr, "Error: Unpaired argument: %s\n", argv[i]);
			freeConfig(&config);
			config.flags = NULL;
			return config;
		}
	}
	
	if (set) {
		fprintf(stderr, "Error: Flag without value\n");
		freeConfig(&config);
		config.flags = NULL;
		return config;
	}
	
	config.flags[flag_count] = '\0'; // Null terminate the flags string
	
	return config;
}

char* getConfigFile(CONFIG* config) {
	if (!config->flags || !config->values) {
		return NULL;
	}
	
	// Look for 'w' flag
	for (int i = 0; i < config->flag_count; i++) {
		if (config->flags[i] == 'w') {
			return config->values[i]; // Return the corresponding value
		}
	}
	
	return NULL; // -w flag not found
}

char* getCommandPath(CONFIG* config) {
	if (!config->flags || !config->values) {
		return NULL;
	}
	
	// Look for 'C' flag
	for (int i = 0; i < config->flag_count; i++) {
		if (config->flags[i] == 'C') {
			return config->values[i]; // Return the corresponding value
		}
	}
	
	return NULL; // -C flag not found
}

bool isSilentMode(CONFIG* config) {
	if (!config->flags) {
		return false;
	}
	
	// Look for 'S' flag
	for (int i = 0; i < config->flag_count; i++) {
		if (config->flags[i] == 'S') {
			return true;
		}
	}
	
	return false; // -S flag not found
}

void freeConfig(CONFIG* config) {
	if (!config) return;
	
	if (config->flags) {
		for (int i = 0; i < config->flag_count; i++) {
			free(config->values[i]);
		}
		free(config->values);
		free(config->flags);
	}
	
	config->flags = NULL;
	config->values = NULL;
	config->flag_count = 0;
}