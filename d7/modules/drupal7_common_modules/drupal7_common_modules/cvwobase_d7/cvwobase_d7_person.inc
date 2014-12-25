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

/**
 * @file
 * Handles the abstract CVWO Person object.
 */

namespace CVWO\Base {
	drupal_load('module', 'upload_qc');
	drupal_load('module', 'jquery_update');
	use \CVWO\Base\Database as db;
	
	/**
	 * Implements hook_requirements for the Person sub-module.
	 * 
	 * @param string $phase The phase that is triggered by Drupal.
	 * @param array $reqs The requirements that will be returned to Drupal as
	 *                    having been unmet.
	 */
	function person_hook_requirements($phase, &$reqs)
	{
		if ($phase === 'runtime')
		{
			$jquery_version = variable_get('jquery_update_jquery_version', '1.5');
			$jquery_version = explode('.', $jquery_version);
			if ($jquery_version[0] < 1 || $jquery_version[1] < 8)
			{
				$reqs[namespace\MODULE_NAME . 'jquery_update'] = array(
					'title'			 => t('jQuery Version out-of-date.'),
					'severity'		 => REQUIREMENT_ERROR,
					'description'	 => t('The jQuery version Drupal is using '
						. 'is too old. Go to the <a href="@config">jQuery '
						. 'Update configuration page</a> to change the'
						. '"Default jQuery Version" to at least 1.8.',
						array('@config' => url('admin/config/development/jquery_update')))
				);
			}
			
			$upload_check = variable_get('upload_qc_error');
			if ($upload_check === null)
			{
				$reqs[namespace\MODULE_NAME . 'upload_qc'] = array(
					'title'			 => t('Upload QC Checking disabled'),
					'severity'		 => REQUIREMENT_WARNING,
					'description'	 => t('Upload QC is not validating uploads.'
						. 'Go to the <a href="@config">Upload QC configuration '
						. 'page</a> to change the "Check for PDO upload errors"'
						. 'setting.',
						array('@config' => url('admin/config/services/upload_qc')))
				);
			}
		}
	}

	/**
	 * Handles the profile photo request.
	 *
	 * @param int $id The ID of the person to get the profile picture of.
	 */
	function person_photo($id)
	{
		$query = db\select(CVWO_PERSON_TABLE)
			->condition('person_id', $id);
		$query->addField(CVWO_PERSON_TABLE, 'photograph');
		$query = $query->execute();
		if (!$query)
		{
			return MENU_NOT_FOUND;
		}

		$row = $query->fetchAssoc();
		if (empty($row) || empty($row['photograph']))
		{
			return MENU_NOT_FOUND;
		}

		$photo_file = file_load($row['photograph']);
		if (!$photo_file)
		{
			return MENU_NOT_FOUND;
		}

		//Set to expire every 24 hours
		drupal_add_http_header('Cache-Control', 'private, max-age=86400');
		drupal_add_http_header('Content-type', $photo_file->filemime);
		drupal_add_http_header('Content-length', $photo_file->filesize);
		$file = fopen(drupal_realpath($photo_file->uri), 'rb');
		fpassthru($file);
		fclose($file);
		drupal_exit();
	}

	/**
	 * Searches for a person using either an incomplete NRIC or partial name.
	 *
	 * @param string $text The name or NRIC fragment to search for.
	 * @param function $modifier A function callback to use to modify the query
	 *                           before execute is called.
	 */
	function person_search_callback($text, $modifier = null)
	{
		$search = sprintf('%%%s%%', $text);
		$query = db\select(PERSON_TABLE, 'p')
			->condition(
				db\or_()
					->condition('name', $search, 'LIKE')
					->condition('nric_fin', $search, 'LIKE'))
			->condition('p.is_deleted', 0)
			->fields('p', array('person_id'));

		//Execute the modifer before executing the statement.
		if (!empty($modifier))
		{
			$modifier($query);
		}
		$query = $query->execute();

		$person_rows = $query->fetchAll();

		$result = array();
		foreach ($person_rows as &$row)
		{
			$person = Person::from_id($row->person_id);
			$row = array(
				'person_id'	 => $person->get_id(),
				'nric_fin'	 => $person->get_nric_fin(),
				'name'		 => $person->get_name(),
				'address'	 => $person->get_addresses(),
				'contact'	 => $person->get_contacts()
			);

			$result[drupal_json_encode($row)] = $row['name'] .
				(empty($row['nric_fin']) ? '' : sprintf(' (%s)', $row['nric_fin']));
		}

		drupal_json_output($result);
	}

	/**
	 * The Abstract CVWO person class. Instead of directly querying the database,
	 * use the getter methods from this class.
	 */
	class Person
	{
		/**
		 * The database ID of this person.
		 * 
		 * @var integer
		 */
		protected $person_id;
		
		/**
		 * The Drupal user ID for this person, or null if none is linked.
		 * 
		 * @var integer
		 */
		protected $uid;

		/**
		 * The name of the person.
		 *
		 * @var string
		 */
		protected $name;

		/**
		 * The NRIC/FIN of the person
		 *
		 * @var string
		 */
		protected $nric_fin;

		/**
		 * The address IDs of this person.
		 *
		 * @var array
		 */
		protected $address_ids = array();

		/**
		 * @param integer $person_id
		 */
		protected function __construct($person_id)
		{
			$query = db\select(PERSON_TABLE, 'p')
				->condition('person_id', $person_id)
				->fields('p', array('person_id', 'uid', 'nric_fin', 'name'))
				->execute();

			if ($query->rowCount() !== 1)
			{
				throw new \InvalidArgumentException('Invalid person ID');
			}

			$person_rows = $query->fetchAssoc();
			$this->person_id = $person_rows['person_id'];
			$this->uid = $person_rows['uid'];
			$this->nric_fin = $person_rows['nric_fin'];
			$this->name = $person_rows['name'];

			//Get the address information
			$this->address_ids = db\select(PERSONAL_PARTICULARS_PERSON_ADDRESS_TABLE, 'pa')
				->fields('pa', array('address_id'))
				->condition('person_id', $this->person_id)
				->execute()
				->fetchAll();
			foreach ($this->address_ids as &$address)
			{
				$address = $address->address_id;
			}
		}

		/**
		 * Creates a new person from their database ID.
		 * 
		 * @param integer $person_id
		 */
		public static function from_id($person_id)
		{
			return new Person($person_id);
		}

		/**
		 * Creates a new person from their NRIC.
		 *
		 * @param string $nric
		 */
		public static function from_nric($nric)
		{
			$query = db\select(PERSON_TABLE, 'p')
				->condition('nric_fin', $nric)
				->condition('is_deleted', 0)
				->fields('p', array('person_id'))
				->execute();

			if ($query->rowCount() !== 1)
			{
				throw new \InvalidArgumentException('Invalid NRIC');
			}

			return new Person($query->fetchField());
		}
		
		/**
		 * Creates a new person given their Drupal user account.
		 * 
		 * @param integer $uid
		 */
		public static function from_uid($uid)
		{
			$query = db\select(PERSON_TABLE, 'p')
				->condition('uid', $uid)
				->condition('is_deleted', 0)
				->fields('p', array('person_id'))
				->execute();

			if ($query->rowCount() !== 1)
			{
				throw new \InvalidArgumentException('Invalid Drupal User ID');
			}

			return new Person($query->fetchField());
		}

		/**
		 * Checks if the person with the given ID exists.
		 *
		 * @param integer $person_id
		 * @return boolean
		 */
		public static function exists($person_id)
		{
			$query = db\select(PERSON_TABLE, 'p')
				->condition('person_id', $person_id)
				->countQuery()
				->execute();

			return intval($query->fetchField()) === 1;
		}

		/**
		 * Checks if the person with the given NRIC exists.
		 * @param string $nric The NRIC to look up.
		 */
		public static function nric_exists($nric)
		{
			$query = db\select(PERSON_TABLE, 'p')
				->condition('nric_fin', $nric)
				->condition('is_deleted', 0)
				->countQuery()
				->execute();

			return intval($query->fetchField()) === 1;
		}

		/**
		 * Checks if the person with the given Drupal User ID exists.
		 * @param integer $uid The Drupal UID to look up.
		 */
		public static function uid_exists($uid)
		{
			$query = db\select(PERSON_TABLE, 'p')
				->condition('uid', $uid)
				->condition('is_deleted', 0)
				->countQuery()
				->execute();

			return intval($query->fetchField()) === 1;
		}

		/**
		 * Gets the Database ID of this person.
		 *
		 * @return integer
		 */
		public function get_id()
		{
			return $this->person_id;
		}

		/**
		 * Gets the NRIC/FIN Number of this person.
		 *
		 * @return string
		 */
		public function get_nric_fin()
		{
			return $this->nric_fin;
		}

		/**
		 * Gets the name of this person.
		 *
		 * @return string
		 */
		public function get_name()
		{
			return $this->name;
		}

		/**
		 * Gets the addresses of the person.
		 *
		 * @return array
		 */
		public function get_addresses()
		{
			$result = array();
			$query = db\select(CVWO_ADDRESS_TABLE, 'a')
				->fields('a', array(
					'address_id',
					'blk_no',
					'street_name',
					'building_name',
					'floor_no',
					'unit_no',
					'postal_code'))
				->condition('address_id', $this->address_ids)
				->execute();

			while (($row = $query->fetchAssoc()) !== false)
			{
				//Build the display text
				$row['display_text'] = self::format_address($row);
				$result[] = $row;
			}

			return $result;
		}

		/**
		 * Gets the contact information of the person.
		 *
		 * @return array An array containing 3 values: contact_type, contact_value,
		 *         and contact_type_display. contact_type is a CodeM value.
		 */
		public function get_contacts()
		{
			$result = db\select(CVWO_PERSON_CONTACT_TABLE, 'pc')
				->fields('pc', array('contact_type', 'contact_value'))
				->condition('person_id', $this->person_id)
				->execute()
				->fetchAll();
			foreach ($result as &$contact)
			{
				$contact = (array)$contact;
				$contact['contact_type_display'] = cvwocodem2_getanstext($contact['contact_type']);
			}

			return $result;
		}
		
		/**
		 * Deletes the current person from the database.
		 * 
		 * This must be wrapped around a database transaction.
		 */
		public function delete()
		{
			assert(db\Transaction::is_in_transaction());
			db\update(CVWO_PERSON_TABLE)
				->fields(array('is_deleted' => 1, 'user_modified' => $GLOBALS['user']->uid))
				->condition('person_id', $this->person_id)
				->execute();
		}

		/**
		 * Formats a CVWO Address table record for display.
		 *
		 * @param array $address One record from the database
		 */
		public static function format_address($address)
		{
			$result = '';
			if ($address['blk_no']) {
				$result .= $address['blk_no'] . ' ';
			}
			if ($address['street_name']) {
				$result .= $address['street_name'] . "\n";
			}
			if ($address['building_name']) {
				$result .= $address['building_name'] . "\n";
			}
			if ($address['floor_no'] && $address['unit_no']) {
				$result .= '#' . $address['floor_no'] . '-' . $address['unit_no'] . "\n";
			}
			if ($address['postal_code']) {
				$result .= $address['postal_code'] . ' ';
			}
			$result .= 'Singapore';

			return $result;
		}
		
		/**
		 * Validates a NRIC number.
		 * 
		 * @param string $NRIC The NRIC to validate.
		 * @param boolean $is_local Set to true to restrict only to local NRIC,
		 *        set to false to restrict to FIN, leave blank for either.
		 * @return boolean True if the NRIC is valid.
		 */
		public static function validate_nric($NRIC, $is_local = null)
		{
			if (strlen($NRIC) !== 9 || !is_numeric(substr($NRIC, 1, 7)))
			{
				//NRIC must be 9 characters long, and the middle 7 characters must
				//be numeric.
				return false;
			}

			//Normalise our input NRIC to be uppercase.
			$NRIC		 = strtoupper($NRIC);
			$checksum	 = 0;

			//Verify the first alphabet, if we have a restriction on the type of
			//ICs we want.
			if (!is_null($is_local))
			{
				if ($is_local && !in_array($NRIC[0], array('S', 'T'), true))
				{
					//Only NRICs
					return false;
				}
				else if (!in_array($NRIC[0], array('F', 'G'), true))
				{
					//Only FINs
					return false;
				}
			}

			//Initialise our checksum algorithm.
			switch ($NRIC[0])
			{
				case 'T':
					$checksum	 = 4;
					//fall through
				case 'S':
					$letters	 = 'ABCDEFGHIZJ';
					break;

				case 'G':
					$checksum	 = 4;
					//fall through
				case 'F':
					$letters	 = 'KLMNPQRTUWX';
					break;

				default:
					//Invalid opening character.
					return false;
			}

			//Multiply the weights associated with each digit
			$multipliers = array(0, 2, 7, 6, 5, 4, 3, 2);
			for ($i = 1; $i < 8; ++$i)
			{
				$checksum += ($multipliers[$i] * intval($NRIC[$i]));
			}

			//Check the remainder
			$checksum = 10 - ($checksum % 11);
			return $NRIC[8] === $letters[$checksum];
		}
	}
}
