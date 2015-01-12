AUTHOR: Chen Yuling
PERSON IN CHARGE: Yuling
TESTING: Name/Date,
Code Review: Name/Date, 

MODULE: Hello World (D7)

DESCRIPTION: Hello World contains the following features/learning points:
 - Add/View/Edit/Delete/Search user records
 - Block
 - Form theming
 - Help
 - Autocomplete
 - Admin (usage of variable_get and variable_set)
 - Use of Query objects (Select, Update, Count, Pager) in D7
 - Some CVWO conventions (Audit trail, permissions etc.)

DESCRIPTION OF INCLUDE FILES:
 - helloworld_d7.js
	- Contains js needed for confirm to pop out
 - helloworld_d7_api.inc
	- API for loading, adding, updating, deleting, searching, counting user records
 - helloworld_d7_constants.php
	- Contains the constants needed in this module. Includes constant names for variables, permissions etc.
 - helloworld_d7_form.inc
	- Form for adding/editing/viewing/deleting a record
 - helloworld_d7_list.inc
	- Form for searching records