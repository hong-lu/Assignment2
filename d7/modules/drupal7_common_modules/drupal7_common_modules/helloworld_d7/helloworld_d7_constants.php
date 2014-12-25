<?php

namespace CVWO\HelloWorld {
	// Modules
	const MODULE_NAME = 'helloworld_d7';

	// Permissions
	const SETTINGS_ADMIN_PERM = 'access admin settings';
	const ADD_USER_PERM = 'add user perm';
	const EDIT_USER_PERM = 'edit user perm';
	const VIEW_USER_PERM = 'view user perm';
	const DELETE_USER_PERM = 'delete user perm';
	const LIST_USER_PERM = 'list users perm';

	// Table
	const HELLOWORLD_TABLE = 'helloworld_table';

	// Settings variables
	const PAGELIMIT = 'helloworld_pagelimit';
	const AUTOCOMPLETELIMIT = 'helloworld_autocompletelimit';

	// Audit Logs
	const ADD_AUDIT = 'Added user record';
	const EDIT_AUDIT = 'Edited user record';
	const DELETE_AUDIT = 'Deleted user record';
	const LOAD_AUDIT ='Loaded user record';
}
