<?php
// Tables
define('CVWO_REPORT_SETUP_TABLE', 'cvwo_report_setup');
define('CVWO_REPORT_TABLE', 'cvwo_report');
define('CVWO_REPORT_ROLES_TABLE', 'cvwo_report_roles');
define('CVWO_REPORT_USERS_TABLE', 'cvwo_report_users');
define('CVWO_REPORT_QUERIES_TABLE', 'cvwo_report_queries');
define('CVWO_REPORT_SPECIAL_TABLE', 'cvwo_report_special');
define('CVWO_REPORT_CATEGORY_TABLE', 'cvwo_report_category');

// Permission:
// Only one permission set for the administrator or users
// given this permission to add reports and grant rights
// to show/delete/edit reports
// Reports list will be extracted out according to the permissions
// set to the current user
define('CVWO_REPORT_SETUP_PERM', 'Setup/edit localization information to integrate the report module into specific environment');
define('CVWO_REPORT_ADD_PERM', 'Add reports/duplicate reports and attach them to roles and/or users');
define('CVWO_REPORT_ADD_SPECIAL_PERM', 'Add/edit special reports and attach them to roles and/or users');
define('CVWO_REPORT_NORMAL_ACCESS_PERM', 'View/Show/Edit/Delete reports authorized');
define('CVWO_REPORT_MANAGE_REPORTS_PERM', 'Edit Reports\' weights and categories');
define('CVWO_REPORT_MANAGE_CATEGORIES_PERM', 'View/Show/Edit/Delete categories for reports');

// Permission for each report:
define('CVWO_REPORT_NO_PERMISSION', '0');
define('CVWO_REPORT_VIEW_ONLY_PERM', '1');
define('CVWO_REPORT_VIEW_AND_EDIT_PERM', '2');
define('CVWO_REPORT_VIEW_EDIT_AND_DELETE_PERM', '3');

// Setup constant
define('CVWO_REPORT_DEFAULT_SETUP_ID', 1);
define('CVWO_REPORT_DEFAULT_CATEGORY_ID', 1);
define('CVWO_REPORT_DEFAULT_REPORT_ID', 0);

// Tags
define('CVWO_REPORT_TAG_ADD', 'add');
define('CVWO_REPORT_TAG_DELETE', 'delete');
define('CVWO_REPORT_TAG_EDIT', 'edit');

define('CVWO_REPORTS_ACCESS_QUESTION', 'Access control');
define('CVWO_REPORTS_ACCESS_QUESTION_ID', 1);