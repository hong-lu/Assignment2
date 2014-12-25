<?php
/*
 * The MIT License
 *
 * Copyright 2007-2013 Computing for Volunteer Welfare Organizations (CVWO),
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

namespace CVWO\Base {

	use \CVWO\Base\Database as db;

	function organisation_delete(array $form, array &$form_state, $organisation_id = null) {
		if (!Organisation::exists($organisation_id)) {
			drupal_not_found();
			drupal_exit();
		}

		$form = array();
		$form['#deleted_id'] = $organisation_id;
		$organisation = Organisation::from_id($organisation_id);
		return confirm_form($form, t('Are you sure you want to delete the organisation @organisation?',
			array(
				'@organisation' => $organisation->get_name()
			)),
			MODULE_NAME . '/organisation', t('This action cannot be undone.'), t('Delete'));
	}

	function organisation_delete_submit(array $form, array &$form_state) {
		delete_organisation($form['#deleted_id']);

		$form_state['redirect'] = MODULE_NAME . '/organisation';
	}

	function delete_organisation($organisation_id) {
		global $user;
		$current_user_id = $user->uid;

		$transaction = db\transaction();

		$delete_fields = array('is_deleted' => 1,
			'user_modified' => $current_user_id);

		// mark deletion in organisation_table
		$affected_rows = db\update(ORGANISATION_TABLE)
			->fields($delete_fields)
			->condition('organisation_id', $organisation_id)
			->execute();

		// mark deletion in organisation addresses table
		 db\update(ORGANISATION_ADDRESS_TABLE)
			->fields($delete_fields)
			->condition('organisation_id', $organisation_id)
			->execute();

		// mark deletion in organisation contact persons table
		db\update(ORGANISATION_CONTACT_PERSON_TABLE)
			->fields($delete_fields)
			->condition('organisation_id', $organisation_id)
			->execute();

		if ($affected_rows === 1) {
			$now = new \DateObject();
			$transaction->commit(ORGANISATION_DELETE_LOG_MESSAGE, ORGANISATION_TAG, MODULE_NAME, array(
				'@organisation' => 'organisation_name',
				'@time' => $now->format(\CVWO\Base\DATE_FORMAT_DATETIME)));
			drupal_set_message(t('Organisation deleted successfully!'));
			return true;
		} else {
			drupal_set_message(t('Organisation does not exist! Deletion failed'), 'error');
			return false;
		}
	}
	
	/**
	 * Searches for an organisation using a partial name.
	 *
	 * @param string $text The name of the organisation to search for.
	 * @param function $modifier A function callback to use to modify the query
	 *                           before execute is called.
	 */
	function organisation_search_callback($text, $modifier = null)
	{
		$search = sprintf('%%%s%%', $text);
		$query = db\select(ORGANISATION_TABLE, 'o')
			->condition('name', $search, 'LIKE')
			->condition('is_deleted', 0)
			->fields('o', array('organisation_id'));

		//Execute the modifer before executing the statement.
		if (!empty($modifier))
		{
			$modifier($query);
		}
		$query = $query->execute();

		$organisation_rows = $query->fetchAll();

		$result = array();
		foreach ($organisation_rows as &$row)
		{
			$organisation = Organisation::from_id($row->organisation_id);
			$row = array(
				'organisation_id'	 => $organisation->get_id(),
				'name'				 => $organisation->get_name(),
			);

			$result[drupal_json_encode($row)] = $row['name'];
		}

		drupal_json_output($result);
	}
	
	class Organisation
	{
		/**
		 * The ID of the organisation.
		 * @var integer
		 */
		private $organisation_id;
		
		/**
		 * The name of the organisation.
		 * @var string
		 */
		private $name;

		/**
		 * @param integer $organisation_id This ID is assumed to be valid.
		 */
		protected function __construct($organisation_id)
		{
			$this->organisation_id = $organisation_id;
			$query = db\select(ORGANISATION_TABLE, 'o')
				->condition('o.organisation_id', $organisation_id)
				->fields('o', array('name'))
				->execute();
			$row = $query->fetchAssoc();

			$this->name = $row['name'];
		}

		/**
		 * Creates a new Organisation from their database ID.
		 * 
		 * @param integer $organisation_id
		 */
		public static function from_id($organisation_id)
		{
			if (!Organisation::exists($organisation_id))
			{
				throw new \InvalidArgumentException('The organisation specified does not exist.');
			}

			return new Organisation($organisation_id);
		}

		/**
		 * Checks if the Organisation with the given ID exists.
		 *
		 * @param integer $organisation_id
		 * @return boolean
		 */
		public static function exists($organisation_id)
		{
			$query = db\select(ORGANISATION_TABLE, 'o')
				->condition('organisation_id', $organisation_id)
				->countQuery()
				->execute();

			return intval($query->fetchField()) === 1;
		}

		/**
		 * Gets the Database ID of this organisation.
		 *
		 * @return integer
		 */
		public function get_id()
		{
			return $this->organisation_id;
		}
		
		/**
		 * Gets the name of the organisation.
		 * 
		 * @return string
		 */
		public function get_name()
		{
			return $this->name;
		}
	}
}
