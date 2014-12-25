<?php

/*
 * The MIT License
 *
 * Copyright 2013 Computing for Volunteer Welfare Organizations (CVWO),
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

namespace CVWO\Community {
	/**
	 * The module name.
	 */

	const MODULE_NAME = 'cvwo_community';
	
	const BASE_MODULE_NAME = 'cvwobase_d7';

	/**
	 * Whether to enable debugging features. Do not enable in production!
	 */
	const DEBUG = \CVWO\Base\DEBUG;
	
	/**
	 * The permission.
	 */
	const PERMISSION_PERSON_VIEW = \CVWO\Base\VIEW_PERM;
	const PERMISSION_PERSON_ADD = \CVWO\Base\ADD_PERM;
	const PERMISSION_PERSON_EDIT = \CVWO\Base\EDIT_PERM;
	const PERMISSION_AAS_VIEW = 'view aas';
	const PERMISSION_AAS_ADD = 'add aas';
	const PERMISSION_AAS_EDIT = 'edit aas';
	const PERMISSION_AAS_DELETE = 'delete aas';
	const PERMISSION_INFO_REF_VIEW = 'view info ref';
	const PERMISSION_INFO_REF_ADD = 'add info ref';
	const PERMISSION_INFO_REF_EDIT = 'edit info ref';
	const PERMISSION_INFO_REF_DELETE = 'delete info ref';
	const PERMISSION_HOME_VISIT_VIEW = 'view home visit';
	const PERMISSION_HOME_VISIT_ADD = 'add home visit';
	const PERMISSION_HOME_VISIT_EDIT = 'edit home visit';
	const PERMISSION_HOME_VISIT_DELETE = 'delete home visit';
	
	/**
	 * Extends the abstract CVWO Person to augment with community information
	 * such as living conditions etc.
	 */
	const PERSON_TABLE = 'cvwo_community_person';

	/**
	 * Stores whether a CVWO Person is at risk, over time.
	 */
	const PERSON_AT_RISK_TABLE = 'cvwo_community_person_at_risk';
	
	/**
	 * @subpackage Community Person constants
	 */
	const PERSON_ADD_LOG_MESSAGE = 'Added person';
	const PERSON_PERSONAL_PARTICULARS_TAG = 'Personal Particulars';
	const PERSON_LIVING_PATTERN_QUESTION = 'Living Pattern';
	const PERSON_FINANCIAL_SITUATION_QUESTION = 'Financial Situation';
	const PERSON_INCOME_QUESTION = 'Approximate Monthly Income';
	const PERSON_AT_RISK_QUESTION = 'At Risk';
	const PERSON_OTHER_HEALTH_CONDITION_QUESTION = 'Other Health Condition';
	const PERSON_OTHER_HEALTH_CONDITION_FRAIL = 'Frail';
	const PERSON_OTHER_HEALTH_CONDITION_HOMEBOUND = 'Homebound';
	const PERSON_MOVEMENT_ABILITY_QUESTION = 'Movement Ability';
	const PERSON_EYESIGHT_QUESTION = 'Eyesight';
	const PERSON_HEARING_QUESTION = 'Hearing';
	const PERSON_FINANCIAL_SITUATION_TABLE = 'cvwo_community_person_financial_situation';
	const PERSON_OTHER_HEALTH_CONDITION_TABLE = 'cvwo_community_person_health_condition';

	/**
	 * @subpackage Worker constants
	 */
	const WORKER_TABLE = 'cvwo_community_worker';

	/**
	 * @subpackage AAS constants
	 */
	const AAS_PERSON_TABLE = 'cvwo_community_person_aas';
	const AAS_ITEM_QUESTION = 'AAS Item';
	const AAS_REPORTS_TABLE = 'cvwo_community_aas_reports';
	const AAS_REPORT_TAG = 'AAS Report';
	const AAS_REPORT_ADDED_MESSAGE = 'AAS Report Added: @person at @time';
	const AAS_REPORT_UPDATED_MESSAGE = 'AAS Report Updated: @person at @time';
	const AAS_REPORT_DELETED_MESSAGE = 'AAS Report Deleted: @person at @time';
	/**
	 * @subpackage info_ref constants
	 */
	const INFO_REF_REPORT_TABLE = 'cvwo_community_info_ref_report';
	const INFO_REF_TYPE_QUESTION = 'Referral Type';
	const INFO_REF_STATUS_QUESTION = 'Referral Status';
	const INFO_REF_INFO_OR_REF_QUESTION = 'Info or Referral';
	const INFO_REF_FOLLOW_UP_QUESTION = 'Info/Referral follow-up required';
	const INFO_REF_TAG = 'Info/Referral Report';
	const INFO_REF_ADDED_MESSAGE = 'Info/Referral Added: @person at @time';
	const INFO_REF_UPDATED_MESSAGE = 'Info/Referral Updated: @person at @time';
	const INFO_REF_DELETED_MESSAGE = 'Info/Referral Report Deleted: @person at @time';

	/**
	 * @subpackage home_visit constants
	 */
	const HOME_VISIT_REPORT_TABLE = 'cvwo_community_home_visit';
	const HOME_VISIT_REPORT_WORKER_TABLE = 'cvwo_community_home_visit_report_workers';
	const HOME_VISIT_REPORT_HOME_CONDITION_TABLE = 'cvwo_community_home_visit_report_home_condition';
	const HOME_VISIT_HOME_CONDITION_QUESTION = 'Home Condition';
	const HOME_VISIT_REQUIRES_ATTENTION_QUESTION = 'Requires Attention';
	const HOME_VISIT_TAG = 'Home Visit Report';
	const HOME_VISIT_ADDED_MESSAGE = 'Home Visit Report Added: @person at @time';
	const HOME_VISIT_UPDATED_MESSAGE = 'Home Visit Report Updated: @person at @time';
	const HOME_VISIT_DELETED_MESSAGE = 'Home Visit Report Deleted: @person at @time';

}
