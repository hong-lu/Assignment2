<?php
/*
 * The MIT License
 *
 * Copyright 2011-2013 Computing for Volunteer Welfare Organizations (CVWO),
 * National University of Singapore.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// This namespace is for new constants. Put them ALL here!
namespace CVWO\Base {
	/**
	 * The name of this module.
	 */
	const MODULE_NAME = 'cvwobase_d7';

	/**
	 * If we are in development mode. Debug features will be enabled.
	 *
	 * NEVER release code into production with this set to true!
	 */
	const DEBUG = false;

	/**
	 * Permission to add new people.
	 */
	const ADD_PERM = 'Add Person Records';

	/**
	 * Permission to view people's information.
	 */
	const VIEW_PERM = 'View Person Records';

	/**
	 * Permission to edit information.
	 */
	const EDIT_PERM = 'Edit Person Records';

	/**
	 * Permission to delete people.
	 */
	const DEL_PERM = 'Delete Person Records';

	/**
	 * The Organisation table
	 */
	const ORGANISATION_TABLE = 'cvwo_organisation';

	/**
	 * The Abstract Person table.
	 */
	const PERSON_TABLE = 'cvwo_person';

	/**
	 * This defines a date/time format for all database queries to use.
	 */
	const DATE_FORMAT_DATABASE = 'Y-m-d H:i:s';

	/**
	 * This defines a date/time format for all date_popup modules to use.
	 */
	const DATE_FORMAT_DATETIME = 'D, d/m/Y - g:i a';

	/**
	 * This defines a date format for all date_popup fields to use.
	 */
	const DATE_FORMAT_DATE = 'd/m/Y';

	/**
	 * This defines a time format for all date_popup fields to use.
	 */
	const DATE_FORMAT_TIME = 'g:i a';

	/**
	 * @subpackage Personal particulars form constants.
	 */
	const PERSON_PERSONAL_PARTICULARS_TAG = 'Personal Particulars';
	const PERSON_EDIT_TAG = 'Edited person';
	const PERSON_DELETE_TAG = 'Deleted person';
	const PERSON_ADD_LOG_MESSAGE = 'Added %person';
	const PERSONAL_PARTICULARS_STYLESHEET_PATH = 'cvwobase_d7_person.css';

	/**
	 * The path to the profile photo store.
	 */
	const PERSONAL_PARTICULARS_PROFILE_PHOTO_PATH = 'private://cvwobase_d7/people/photos/';

	const PERSONAL_PARTICULARS_NRIC_COLOUR_QUESTION = 'NRIC Colour';
	const PERSONAL_PARTICULARS_GENDER_QUESTION = 'Gender';
	const PERSONAL_PARTICULARS_SALUTATION_QUESTION = 'Salutation';
	const PERSONAL_PARTICULARS_MARITAL_STATUS_QUESTION = 'Marital Status';
	const PERSONAL_PARTICULARS_MARITAL_STATUS_SEPARATED = 'Separated';
	const PERSONAL_PARTICULARS_MARITAL_STATUS_DIVORCED = 'Divorced';
	const PERSONAL_PARTICULARS_MARITAL_STATUS_MARRIED = 'Married';
	const PERSONAL_PARTICULARS_MARITAL_STATUS_SINGLE = 'Single';
	const PERSONAL_PARTICULARS_MARITAL_STATUS_WIDOWED = 'Widowed';
	const PERSONAL_PARTICULARS_MARITAL_STATUS_COHABITING = 'Cohabiting';
	const PERSONAL_PARTICULARS_NATIONALITY_QUESTION = 'Nationality';
	const PERSONAL_PARTICULARS_PLACE_OF_BIRTH_QUESTION = 'Place of Birth';
	const PERSONAL_PARTICULARS_RACE_QUESTION = 'Race';
	const PERSONAL_PARTICULARS_RELIGION_QUESTION = 'Religion';
	const PERSONAL_PARTICULARS_ETHNICITY_QUESTION = 'Ethnicity';
	const PERSONAL_PARTICULARS_OCCUPATION_QUESTION = 'Occupation';
	const PERSONAL_PARTICULARS_OCCUPATION_STATUS_QUESTION = 'Occupation Status';
	const PERSONAL_PARTICULARS_HIGHEST_EDUCATION_LEVEL_QUESTION = 'Highest Education Level';
	const PERSONAL_PARTICULARS_FORM_SAVE_BUTTON_ID = 'Save';
	const PERSONAL_PARTICULARS_BLANK_PHOTO_PATH = '/assets/blank_recent_photo.png';

	/**
	 * @subpackage Personal Particulars Contact form constants.
	 */
	const PERSONAL_PARTICULARS_CONTACT_TYPE_QUESTION = 'Contact Type';
	
	/**
	 * The answer value for Home Telephone numbers.
	 */
	const PERSONAL_PARTICULARS_CONTACT_TYPE_HOME = 'Home Phone';
	
	/**
	 * The answer value for Mobile Phone numbers.
	 */
	const PERSONAL_PARTICULARS_CONTACT_TYPE_MOBILE = 'Mobile Phone';

	/**
	 * @subpackage Personal Particulars Linguistic Ability form constants.
	 */
	const PERSONAL_PARTICULARS_LANGUAGE_QUESTION = 'Language';
	const PERSONAL_PARTICULARS_LANGUAGE_COMPETENCY_QUESTION = 'Language Competency';
	const PERSONAL_PARTICULARS_LANGUAGE_PROFICIENCY_QUESTION = 'Language Proficiency';

	/**
	 * @subpackage Personal Particulars Address form constants.
	 */
	/**
	 * The table storing the addresses of people.
	 */
	const PERSONAL_PARTICULARS_PERSON_ADDRESS_TABLE = 'cvwo_person_address';
	const PERSONAL_PARTICULARS_ADDRESS_TYPE_QUESTION = 'Address Type';
	const PERSONAL_PARTICULARS_FLAT_TYPE_QUESTION = 'Flat Type';

	/**
	 * @subpackage Personal Particulars Next-of-kin Form constants.
	 */
	const PERSONAL_PARTICULARS_PERSON_NOK_TABLE = 'cvwo_person_nok';
	const PERSONAL_PARTICULARS_NOK_TAG = 'Next-of-Kin';
	const PERSONAL_PARTICULARS_NOK_ADD_MESSAGE = 'Added Next-of-Kin %nok to %person';
	const PERSONAL_PARTICULARS_NOK_RELATION_UPDATE_MESSAGE =
		'Updated next-of-kin information for %person';
	const PERSONAL_PARTICULARS_NOK_RELATIONSHIP_QUESTION = 'Relationship';
	
	/**
	 * @subpackage Organisation
	 */
	
	/** 
	 * Tables storing organisation related information
	 */
	const ORGANISATION_CONTACT_PERSON_TABLE = 'cvwo_organisation_contact_person';
	const ORGANISATION_ADDRESS_TABLE = 'cvwo_organisation_address';
	
	/**
	 * Codem Questions
	 */
	const ORGANISATION_TYPE_QUESTION = 'Organisation Type';
	const ORGANISATION_INDUSTRY_QUESTION = 'Organisation Industry';
	const ORGANISATION_PERSON_DESIGNATION_QUESTION = 'Organisation Contact Person Designation';

	/**
	 * Permission to add new organisation.
	 */
	const ADD_ORG_PERM = 'Add Organisation Records';

	/**
	 * Permission to view organisation's information.
	 */
	const VIEW_ORG_PERM = 'View Organisation Records';

	/**
	 * Permission to edit information.
	 */
	const EDIT_ORG_PERM = 'Edit Organisation Records';

	/**
	 * Permission to delete organisation.
	 */
	const DEL_ORG_PERM = 'Delete Organisation Records';

	/**
	 * Commit messages and tags
	 */
	const ORGANISATION_TAG = 'Organisation';
	const ORGANISATION_EDIT_LOG_MESSAGE = 'Updated @org at @time';
	const ORGANISATION_ADD_LOG_MESSAGE = 'Added @org';
	const ORGANISATION_DELETE_LOG_MESSAGE = 'Deleted @org at @time';
	const ORGANISATION_CONTACT_TAG = 'Organisation Contact Person';
	const ORGANISATION_CONTACT_ADD_MESSAGE = 'Added Organisation Contact Person @org_person to @person';
	const ORGANISATION_CONTACT_UPDATE_MESSAGE ='Updated Organisation Contact Person information for @person';
}

// This namespace is for compatibility with the old code.
namespace {
	use \CVWO\Base as Base;

// Used for indicating the status for Centres, Clients, etc 
// Status Enabled records are active
// Status Disabled records are deleted records
define('STATUS_ENABLED', 1);
define('STATUS_DISABLED', 0);

// Modules
define('CVWOBASE_MODULE', Base\MODULE_NAME);
define('CVWOCODEM_MODULE', 'cvwocodem_d7');
define('CVWOTAN_MODULE', 'cvwotan_d7');
define('CVWOCAL_MODULE', 'cvwocal_d7');
define('CVWOREPORT_MODULE', 'cvworeport_d7');
define('CVWODATAVALIDATE_MODULE', 'cvwodatavalidate_d7');

// Permissions
define('CVWOBASE_D7_SETTINGS_ADMIN_PERM', 'administer cvwobase settings');
define('CVWOBASE_D7_ACCESS_AUDIT_PERM', 'access cvwoaudit log');
define('CVWOBASE_D7_ADMIN_AUDIT_PERM', 'administer log');

// Tables
define('CVWO_AUDIT_TABLE', 'cvwo_audit');
/**
 * Use Base\ORGANISATION_TABLE
 * @deprecated since version 2.0
 */
define('CVWO_ORGANISATION_TABLE', Base\ORGANISATION_TABLE);
/**
 * Use Base\PERSON_TABLE
 * @deprecated since version 2.0
 */
define('CVWO_PERSON_TABLE', Base\PERSON_TABLE);
define('CVWO_ADDRESS_TABLE', 'cvwo_address');

/**
 * Use CVWO\Base\PERSONAL_PARTICULARS_PERSON_ADDRESS_TABLE
 * @deprecated since version 2.0
 */
define('CVWO_PERSON_ADDRESS_TABLE', Base\PERSONAL_PARTICULARS_PERSON_ADDRESS_TABLE);
define('CVWO_PERSON_CONTACT_TABLE', 'cvwo_person_contact');
define('CVWO_PERSON_LANGUAGE_TABLE', 'cvwo_person_language');
/**
 * Use CVWO\Base\PERSONAL_PARTICULARS_PERSON_NOK_TABLE
 * @deprecated since version 2.0
 */
define('CVWO_PERSON_NOK_TABLE', Base\PERSONAL_PARTICULARS_PERSON_NOK_TABLE);
define('CVWO_CENTRE_TABLE', 'cvwo_centre');
define('CVWO_LOCATION_TABLE', 'cvwo_location');

// Language Types
define('CVWO_LANGUAGE_SPOKEN',	'spoken');
define('CVWO_LANGUAGE_WRITTEN',	'written');

// Item types
define('CVWO_ITEM_TYPE_CENTRE', 'centre');
define('CVWO_ITEM_TYPE_PERSON', 'person');
define('CVWO_ITEM_TYPE_LOCATION', 'location');
define('CVWO_ITEM_TYPE_ORGANISATION', 'organisation');

// (Untranslated) Audit Log Form Button Texts
define('CVWOBASE_AUDIT_BTN_VIEW', 'Show Records');
define('CVWOBASE_AUDIT_BTN_DELETE', 'Delete Selected Records');
define('CVWOBASE_AUDIT_BTN_EXPORT', 'Export To Excel');

// Page limit variables
define('PAGELIMIT_CVWOAUDIT', 'cvwoaudit_pagelimit');
define('PAGELIMIT_CVWOAUDIT_DEFAULT', 30);
define('PAGELIMIT_CVWOBASE', 'cvwobase_search_pagelimit');
define('PAGELIMIT_CVWOBASE_SEARCH_DEFAULT', 50);

// Other Variables
define('CVWOBASE_D7_PHP_BINARY', 'cvwobase_phpbinary');
define('CVWOBASE_D7_MAIL_SEND_ASYNC', 'cvwobase_send_async');
define('CVWOBASE_D7_MAIL_HOST', 'cvwobase_mailhost');
define('CVWOBASE_D7_MAIL_USER', 'cvwobase_mailuser');
define('CVWOBASE_D7_MAIL_PASS', 'cvwobase_mailpass');
define('CVWOBASE_D7_MAIL_PORT', 'cvwobase_mailport');

// Variable Defaults
define('CVWOBASE_D7_MAIL_SEND_ASYNC_DEFAULT', TRUE);
define('CVWOBASE_D7_MAIL_HOST_DEFAULT', 'ssl://smtp.gmail.com');
define('CVWOBASE_D7_MAIL_USER_DEFAULT', 'nuscvwo');
define('CVWOBASE_D7_MAIL_PASS_DEFAULT', 'vwo2007vwo2007');
define('CVWOBASE_D7_MAIL_PORT_DEFAULT', 465);

// Queues
define('CVWOBASE_MAIL_QUEUE', 'cvwobase_mails');
define('CVWOBASE_MAIL_ERROR_QUEUE', 'cvwobase_errors');

// Audit Tags
define('CVWOBASE_ADD_CENTRE_AUDIT', 'Added Centre');
define('CVWOBASE_EDIT_CENTRE_AUDIT', 'Edited Centre details');
define('CVWOBASE_DELETE_CENTRE_AUDIT', 'Deleted Centre');

define('CVWOBASE_ADD_LOCATION_AUDIT', 'Added Location');
define('CVWOBASE_EDIT_LOCATION_AUDIT', 'Edited Location details');
define('CVWOBASE_DELETE_LOCATION_AUDIT', 'Deleted Location');

define('CVWOBASE_ADD_ADDRESS_AUDIT', 'Added Address');
define('CVWOBASE_EDIT_ADDRESS_AUDIT', 'Edited Address');
define('CVWOBASE_DELETE_ADDRESS_AUDIT', 'Deleted Address');

define('CVWOBASE_ADD_PERSON_AUDIT', 'Added Person');
define('CVWOBASE_EDIT_PERSON_AUDIT', 'Edited Person');

define('CVWOBASE_ADD_ORGANISATION_AUDIT', 'Added Organisation');
define('CVWOBASE_EDIT_ORGANISATION_AUDIT', 'Edited Organisation');
define('CVWOBASE_DELETE_ORGANISATION_AUDIT', 'Deleted Organisation');

// Watchdog types
define('CVWO_INSTALL_ERROR', 'Installation Error');
}
