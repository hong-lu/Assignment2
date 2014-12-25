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

	use \CVWO\Base\Database as db;

	function community_format_person_name($values) {
		if (Person::exists($values['person_id'])) {
			return Person::from_id($values['person_id'])->get_name();
		} else {
			return '';
		}
	}

	function community_format_worker($values) {
		if (Worker::exists($values['worker_id'])) {
			return Worker::from_id($values['worker_id'])->get_name();
		} else {
			return '';
		}
	}

	function community_format_address($values) {
		$data = db\select(CVWO_ADDRESS_TABLE, 'add_t')
			->condition('address_id', $values['address_id'])
			->fields('add_t')
			->execute()
			->fetchAssoc();

		return Person::format_address($data);
	}

	function commuinty_format_contact($values) {
		if (Person::exists($values['person_id'])) {
			$contacts = Person::from_id($values['person_id'])->get_contacts();
			if (empty($contacts)) {
				return '';
			}
			foreach ($contacts as $key => $value) {
				$result[] = $value['contact_value'];
			}
			return implode($result, '; ');
		} else {
			return '';
		}
	}

	//Formats a date from database datetime type to 'd-m-y'
	function community_format_date_from_database($date) {
		if (!empty($date))
		{
			$date = new \DateTime($date);
			return $date->format(\CVWO\Base\DATE_FORMAT_DATE);
		}
	}
}
