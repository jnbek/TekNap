/*
 * input.h: header for input.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: input.h,v 1.1.1.1 2000/06/22 09:34:40 edwards Exp $
 */

#ifndef __input_h_
#define __input_h_
	char	input_pause (char *);
	void	set_input (char *);
	void	set_input_prompt (Window *, char *, int);
	char	*get_input_prompt (void);
	char	*get_input (void);
	void	update_input (int);
	void	init_input (void);
	void	input_move_cursor (int);
	void	change_input_prompt (int);
	void	cursor_to_input (void);
	void	edit_char(unsigned char);
	
BUILT_IN_COMMAND(typecmd);
	
/* keybinding functions */
	void 	backward_character 	(char, char *);
	void 	backward_history 	(char, char *);
	void 	clear_screen 		(char, char *);
	void	command_completion 	(char, char *);
	void 	forward_character	(char, char *);
	void 	forward_history 	(char, char *);
	void	highlight_off 		(char, char *);
	void	input_add_character 	(char, char *);
	void	input_backspace 	(char, char *);
	void	input_backward_word 	(char, char *);
	void	input_beginning_of_line (char, char *);
	void	new_input_beginning_of_line (char, char *);
	void	input_clear_line 	(char, char *);
	void	input_clear_to_bol 	(char, char *);
	void	input_clear_to_eol 	(char, char *);
	void	input_delete_character 	(char, char *);
	void	input_delete_next_word 	   (char, char *);
	void	input_delete_previous_word (char, char *);
 	void	input_delete_to_previous_space (char, char *);
	void	input_end_of_line 	   (char, char *);
	void	input_forward_word 	   (char, char *);
	void	input_transpose_characters (char, char *);
	void	input_yank_cut_buffer 	   (char, char *);
	void	insert_bold 		(char, char *);
	void	join_lastinvite		(char, char *);
	void	insert_reverse 		(char, char *);
	void	insert_underline 	(char, char *);
	void	insert_blink		(char, char *);
	void	insert_altcharset	(char, char *);
	void 	meta1_char 		(char, char *);
	void 	meta2_char 		(char, char *);
	void 	meta3_char 		(char, char *);
	void 	meta4_char 		(char, char *);
	void 	meta5_char 		(char, char *);
	void 	meta6_char 		(char, char *);
	void 	meta7_char 		(char, char *);
	void 	meta8_char 		(char, char *);
	void 	meta9_char 		(char, char *);
	void 	meta10_char 		(char, char *);
	void 	meta11_char 		(char, char *);
	void 	meta12_char 		(char, char *);
	void 	meta13_char 		(char, char *);
	void 	meta14_char 		(char, char *);
	void 	meta15_char 		(char, char *);
	void 	meta16_char 		(char, char *);
	void 	meta17_char 		(char, char *);
	void 	meta18_char 		(char, char *);
	void 	meta19_char 		(char, char *);
	void 	meta20_char 		(char, char *);
	void 	meta21_char 		(char, char *);
	void 	meta22_char 		(char, char *);
	void 	meta23_char 		(char, char *);
	void 	meta24_char 		(char, char *);
	void 	meta25_char 		(char, char *);
	void 	meta26_char 		(char, char *);
	void 	meta27_char 		(char, char *);
	void 	meta28_char 		(char, char *);
	void 	meta29_char 		(char, char *);
	void 	meta30_char 		(char, char *);
	void 	meta31_char 		(char, char *);
	void 	meta32_char 		(char, char *);
	void 	meta33_char 		(char, char *);
	void 	meta34_char 		(char, char *);
	void 	meta35_char 		(char, char *);
	void 	meta36_char 		(char, char *);
	void 	meta37_char 		(char, char *);
	void 	meta38_char 		(char, char *);
	void 	meta39_char 		(char, char *);
	
	void	refresh_inputline 	(char, char *);
	void 	send_line 		(char, char *);
	void 	toggle_insert_mode 	(char, char *);
	void	input_msgreply		(char, char *);
	void	input_autoreply		(char, char *);

	void	input_msgreplyback	(char, char *);
	void	input_autoreplyback	(char, char *);

	void	my_scrollback		(char, char *);
	void	my_scrollforward	(char, char *);
	void	my_scrollend		(char, char *);

	void	window_swap1		(char, char *);
	void	window_swap2		(char, char *);
	void	window_swap3		(char, char *);
	void	window_swap4		(char, char *);
	void	window_swap5		(char, char *);
	void	window_swap6		(char, char *);
	void	window_swap7		(char, char *);
	void	window_swap8		(char, char *);
	void	window_swap9		(char, char *);
	void	window_swap10		(char, char *);
	void	w_help			(char, char *);
	void	cpu_saver_on		(char, char *);
	void	window_key_balance	(char, char *);
	void	window_grow_one		(char, char *);
	void	window_key_hide		(char, char *);
	void	window_key_kill		(char, char *);
	void	window_key_list		(char, char *);
	void	window_key_move		(char, char *);
	void	window_shrink_one	(char, char *);
	void	nick_completion		(char, char *);
	void	input_unclear_screen	(char, char *);
	void	tab_completion		(char, char *);
	void	parse_text		(char, char *);
	void	quote_char		(char, char *);
	void	type_text		(char, char *);
		
	Lastlog *get_input_hold		(Window *);
	Display *get_screen_hold	(Window *);
	char	*getchannick		(char *, char *);
	void	paste_to_input		(char, char *);

enum completion {
	NO_COMPLETION,
	TABKEY_COMPLETION,
	NICK_COMPLETION,
	COM_COMPLETION,
	CHAN_COMPLETION,
	EXEC_COMPLETION,
	FILE_COMPLETION,
	DCC_COMPLETION,
	LOAD_COMPLETION,
	SERVER_COMPLETION,
	CDCC_COMPLETION
};

	char	*get_completions	(enum completion, char *, int *, char **);
	int	add_completion_type	(char *, int, enum completion);

/* used by update_input */
#define NO_UPDATE 0
#define UPDATE_ALL 1
#define UPDATE_FROM_CURSOR 2
#define UPDATE_JUST_CURSOR 3

#endif /* __input_h_ */
