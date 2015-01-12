<?php

namespace CVWO\HelloWorld {

	use \CVWO\Base\Database as db;

	/**
	 * Function loads a user from the id
	 *
	 * @param $form_state
	 *    This will be changed to contain loaded information from the database
	 * @param helloworld_id
	 *    The id of the data that is to be loaded from the database
	 */
	function load_user(&$form_state, $helloworld_id) {
		global $user;
		$username = $user->name;
		$message = $username.' has loading user with helloworld_id: '.$helloworld_id;
		$txn = db\transaction();
		
		try {
			$result = db\select(HELLOWORLD_TABLE, 'h')
				->fields('h', array('name', 'email', 'comment', 'status'))
				->condition('helloworld_id', $helloworld_id, '=')
				->execute()
				->fetchAssoc();
		} catch (Exception $e) {
			$txn->rollback();
			watchdog_exception('type', $e);
			return FALSE;
		}
		$txn->commit($message, LOAD_AUDIT, MODULE_NAME);
		// don't load the data if it is disabled
		if ($result['status'] == STATUS_DISABLED) {
			return FALSE;
		}
		
		$form_state['values']['name'] = $result['name'];
		$form_state['values']['email'] = $result['email'];
		$form_state['values']['comment'] = $result['comment'];
		return TRUE;

	}
	 
	/**
	 * Function adds a user
	 *
	 * @param $values
	 *    Will contain the array of values needed to add a new row inside 
	 * the Helloworld table. I.e. $values['name'], $values['email'], $values['comment']
	 */
	function add_user($values) {
		global $user;
		$username = $user->name;
		$message = $username . ' added a user: ' . $values['name'];
		$txn = db\transaction();
		
		try {
			$id = db\insert(HELLOWORLD_TABLE)
				->fields(array(
					'name' => $values['name'],
					'email' => $values['email'],
					'comment' => $values['comment'],
				))
				->execute();
		} catch (Exception $e) {
			$txn->rollback();
			watchdog_exception('type', $e);
			return FALSE;
		}

		$txn->commit($message, ADD_AUDIT ,MODULE_NAME);
		return $id;
	}

	/**
	 * Function updates a current user
	 *
	 * @param $form_state
	 *    Will contain the array of values needed to update a row inside 
	 * the Helloworld table I.e. $values['name'], $values['email'], $values['comment']
	 */
	function update_user($values) {
		global $user;
		$username = $user->name;
		$message = $username . ' updated a user: ' . $values['name'];
		$txn = db\transaction();
		
		try {
			$id = db\update(HELLOWORLD_TABLE)
				->fields(array(
					'name' => $values['name'],
					'email' => $values['email'],
					'comment' => $values['comment'],
				))
				->condition('helloworld_id', $values['helloworld_id'], '=')
				->execute();
		} catch (Exception $e) {
			$txn->rollback();
			watchdog_exception('type', $e);
			return FALSE;
		}
		$txn->commit($message, EDIT_AUDIT, MODULE_NAME);
		// here we return true instead of id
		// if 0 rows are altered, if we return id, it will return 0, which will be read as false
		return TRUE;	
	}

	/**
	 * Function deletes a user
	 *
	 * @param helloworld_id
	 *    The id of the data that is to be deleted from the database
	 */
	function delete_user($helloworld_id) {
		global $user;
		$username = $user->name;
		$txn = db\transaction();
		
		try {
			// we get the name first for audit log purposes
			$message = $username . ' deleted a user: ' . get_name($helloworld_id);

			// cvwo convention to update instead of really deleting the entry from the table
			$id = db\update(HELLOWORLD_TABLE)
				->fields(array(
					'status' => STATUS_DISABLED,
				))
				->condition('helloworld_id', $helloworld_id, '=')
				->execute();
		} catch (Exception $e) {
			$txn->rollback();
			watchdog_exception('type', $e);
			return FALSE;
		}
		$txn->commit($message, DELETE_AUDIT, MODULE_NAME);
		return TRUE;
	}

	/**
	 * Function gets the name of the user
	 *
	 * @param helloworld_id
	 *    id whose name we want to get
	 */
	function get_name($helloworld_id) {
		$result = db\select(HELLOWORLD_TABLE, 'h')
			->fields('h', array('name'))
			->condition('helloworld_id', $helloworld_id, '=')
			->execute()
			->fetchAssoc();
		return $result['name'];
	}
	 
	/**
	 * Function that queries database for searched items
	 *
	 * @param name
	 *    Contains the search options
	 */
	function search_output($name = '') {
		$header = array(
			array('data' => 'Name', 'field' => 'name', 'sort' => 'ASC'),
			array('data' => 'Email', 'field' => 'email'),
			array('data' => 'Comments', 'field' => 'comment'),
			array('data' => 'Options'));
			
		$result = db\select(HELLOWORLD_TABLE, 'h')
				->extend('PagerDefault')
				->extend('TableSort')
				->fields('h', array('helloworld_id', 'name', 'email', 'comment'))
				->condition('status', STATUS_ENABLED, '=')
				->condition('name', '%' . $name . '%', 'LIKE')
				->limit(variable_get(PAGELIMIT))
				->orderByHeader($header)
				->execute();
				
		$rows = array();
		$count = 0;
		while ($data = $result->fetchAssoc()) {
			$row = array(
				$data['name'],
				$data['email'],
				$data['comment'],
				get_options($data['helloworld_id']),
			);
			$rows[] = $row;
		}

		return theme('table', array('header' => $header,
								 'rows' => $rows,
								 'empty' => 'No entries found')) .
				theme('pager');
	}

	/**
	 * Function returns options based on user_access
	 *
	 * @param helloworld_id
	 * 	  Returns links to appropriate page based on this id
	 */
	function get_options($helloworld_id) {
		$output = '';
		
		// either edit or view. If have edit already, no need to have view.
		if (user_access(EDIT_USER_PERM)) {
			$output .= l('Edit', MODULE_NAME . "/edit/" . $helloworld_id, array('attributes' => array('target' => '_blank'))); // open the page in a new tab
		} else if (user_access(VIEW_USER_PERM)) {
			$output .= l('View', MODULE_NAME . "/view/" . $helloworld_id, array('attributes' => array('target' => '_blank'))); // open the page in a new tab
		}
		
		if (user_access(DELETE_USER_PERM)) {
			if ($output != '') {
				$output .= ' - ';
			}
			$output .= l('Delete', MODULE_NAME . "/delete/" . $helloworld_id, 
						array('attributes' => 
							array('onclick' => 'return confirm("Are you sure you want to delete this record?")')));
		}
		return $output;
	}

	/**
	 * Function gets no. of users currently in system
	 */
	function get_no_of_users() {
		// note: order is important here; cannot do countQuery() before condition
		$result = db\select(HELLOWORLD_TABLE, 'h')
			->condition('status', STATUS_ENABLED, '=')		
			->countQuery()
			->execute()
			->fetchField();
		// also, can do rowCount(), instead of countQuery()
		return $result;
	}

	/**
	 * Function deletes a user
	 *
	 * @param helloworld_id
	 *    The id to be deleted
	 */
	function delete($helloworld_id) {
		$name = get_name($helloworld_id);
		if (delete_user($helloworld_id)) {
			drupal_set_message(t('%name has been successfully deleted', array('%name' => $name)));
		} else {
			drupal_set_message(t('An error was encountered while trying to delete %name', array('%name' => $name)));
		}
		drupal_goto(MODULE_NAME);
	}

	/**
	 * Autocomplete function for name
	 *
	 * @param str
	 *    String that is typed into the autocomplete field 
	 */  
	function helloworld_name_autocomplete($str = '') {
		$matches = array();

		$result = db\select(HELLOWORLD_TABLE, 'h')
					->fields('h', array('name'))
					->condition('name', '%' . $str . '%', 'LIKE')
					->condition('status', STATUS_ENABLED, '=')
					->orderBy('name', 'ASC')
					->range(0, variable_get(AUTOCOMPLETELIMIT))
					->execute();
		while ($user = $result->fetchAssoc()) {
			$matches[$user['name']] = check_plain($user['name']);
		}
		drupal_json_output($matches);
	}
}
