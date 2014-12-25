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

namespace CVWO\Base {

	use \CVWO\Base\Database as db;

	/**
	 * Builds the person edit form.
	 *
	 * @param array $form The form to build into.
	 * @param array $form_state The current state of the form.
	 * @param integer $id The ID of the person to edit.
	 * @return array The compiled Drupal form.
	 */
	function person_edit_form($form, &$form_state, $id = null)
	{
		try
		{
			$instance = new PersonEditForm($id);
			return $instance->build($form_state);
		}
		catch (\InvalidArgumentException $e)
		{
			drupal_not_found();
			drupal_exit();
		}
	}

	/**
	 * Validates the person edit form.
	 *
	 * @param array $form The Drupal form
	 * @param array $form_state The Drupal state of the form.
	 */
	function person_edit_form_validate(&$form, &$form_state)
	{
		$instance = $form['#instance'];
		$instance->validate($form, $form_state);
	}

	/**
	 * Handles submissions of the person edit form.
	 *
	 * @param array $form The form built.
	 * @param array $form_state The state of the form submittal.
	 */
	function person_edit_form_submit($form, &$form_state)
	{
		$instance = $form['#instance'];
		$instance->process($form_state);
	}

	class PersonEditForm extends Form
	{
		/**
		 * The person this form will be editing.
		 * @var integer
		 */
		private $person_id;
		
		/**
		 * The Drupal UID of the person who created this person.
		 * @var integer
		 */
		private $user_created;
		
		/**
		 * The timestamp of creation of this person.
		 * @var \DateTime
		 */
		private $date_created;
		
		/**
		 * The Drupal UID of the person who last modified this person.
		 * @var integer
		 */
		private $user_modified;
		
		/**
		 * The timestamp of last modification of this person.
		 * @var \DateTime
		 */
		private $date_modified;

		/**
		 * @param integer $person_id The ID of the person to edit.
		 * @throws \InvalidArgumentException If a person ID was provided and the
		 *                                   person does not exist.
		 */
		public function __construct($person_id = null)
		{
			parent::__construct();
			$this->person_id = $person_id;

			//Check if we are editing a person. If we are, and the person doesn't
			//exist, bail out.
			if (!empty($this->person_id))
			{
				$query = db\select(CVWO_PERSON_TABLE, 'p')
					->condition('p.person_id', $this->person_id);
				$query->leftJoin(CVWO_PERSON_TABLE, 'mp', 'p.user_modified=mp.uid');
				$query->leftJoin(CVWO_PERSON_TABLE, 'cp', 'p.user_created=cp.uid');
				$query->fields('p', array(
						'date_modified',
						'date_created'));
				$query->addField('cp', 'person_id', 'user_created');
				$query->addField('mp', 'person_id', 'user_modified');
				$query = $query->execute();
				
				$row = $query->fetchAssoc();
				if ($row === false)
				{
					throw new \InvalidArgumentException(t('The person selected'
						. 'for editing does not exist.'));
				}
				
				$this->date_created = new \DateObject($row['date_created'],
					null, \CVWO\Base\DATE_FORMAT_DATABASE);
				if ($this->date_created->timeOnly)
				{
					$this->date_created = null;
				}
				$this->user_created = $row['user_created'];
				$this->date_modified = new \DateObject($row['date_modified']);
				if ($this->date_modified->timeOnly)
				{
					$this->date_modified = null;
				}
				$this->user_modified = $row['user_modified'];
			}
		}

		public function serialize()
		{
			return serialize(array(
				$this->person_id,
				$this->user_created,
				$this->date_created,
				$this->user_modified,
				$this->date_modified,
				parent::serialize()
			));
		}
		
		public function unserialize($data)
		{
			$unpack = unserialize($data);
			$this->person_id = $unpack[0];
			$this->user_created = $unpack[1];
			$this->date_created = $unpack[2];
			$this->user_modified = $unpack[3];
			$this->date_modified = $unpack[4];
			parent::unserialize($unpack[5]);
		}

		protected function do_build(&$form_state)
		{
			//Create our form. Encoding type is multipart/form-data to allow uploading
			//the photograph.
			$form = array(
				'#attached'		 => array(
					'css' => array(
						drupal_get_path('module', namespace\MODULE_NAME) . '/' .
							namespace\PERSONAL_PARTICULARS_STYLESHEET_PATH,
					)
				),
				'#attributes'	 => array(
					'class'	=> array(
						namespace\MODULE_NAME . '_person_edit_form'
					)
				)
			);
			
			if (!empty($this->person_id))
			{
				$form['change_history'] = array(
					'#type'				 => 'item',
					'#markup'			 => t('Created by @creator at @created; last '
						. 'modified by @modifier at @modified', array(
							'@creator' => empty($this->user_created) ?
								t('Unknown') : Person::from_id($this->user_created)->get_name(),
							'@created' => empty($this->date_created) ?
								t('Unknown') : $this->date_created->format(\CVWO\Base\DATE_FORMAT_DATETIME),
							'@modifier' => empty($this->user_modified) ?
								t('Unknown') : Person::from_id($this->user_modified)->get_name(),
							'@modified' => empty($this->date_modified) ?
								t('Unknown') : $this->date_modified->format(\CVWO\Base\DATE_FORMAT_DATETIME)
						))
				);
			}

			$personalParticularsForm = new PersonalParticularsForm();
			$this->add_child($personalParticularsForm);
			$form['personal_particulars'] = array_merge(
				array(
					'#type'			 => 'fieldset',
					'#title'		 => t('Personal Particulars'),
					'#collapsible'	 => true
				),
				$personalParticularsForm->build($form_state));

			//Build the other sub-forms
			$contactDetailsForm = new PersonContactDetailsForm();
			$this->add_child($contactDetailsForm);
			$form['contact_details'] = array_merge(
				array(
					'#type'			 => 'fieldset',
					'#title'		 => t('Contact Details'),
					'#collapsible'	 => true
				),
				$contactDetailsForm->build($form_state));
			$languageAbilityForm = new LinguisticAbilitiesForm();
			$this->add_child($languageAbilityForm);
			$form['languages'] = array_merge(
				array(
					'#type'			 => 'fieldset',
					'#title'		 => t('Linguistic Abilities'),
					'#collapsible'	 => true
				),
				$languageAbilityForm->build($form_state));
			$addressDetailsForm = new PersonAddressForm();
			$this->add_child($addressDetailsForm);
			$form['address_details'] = array_merge(
				array(
					'#type'			 => 'fieldset',
					'#title'		 => t('Address Details'),
					'#collapsible'	 => true
				),
				$addressDetailsForm->build($form_state));

			//Information about the person we are editing.
			$form['person_id'] = array(
				'#type'	 => 'hidden',
				'#value' => $this->person_id,
			);

			$form['submit'] = array(
				'#type'	 => 'submit',
				'#value' => t('Save'),
				'#id'	 => namespace\PERSONAL_PARTICULARS_FORM_SAVE_BUTTON_ID,
			);

			$this->entries = array_merge_recursive($this->entries, $form);
		}

		protected function do_process(&$form_state)
		{
			//Store the submitted values into persistent form storage.
			$form_state['storage']['values'] = array_slice($form_state['values'], 0);
			unset($form_state['storage']['values']['add_more_btn'],
				$form_state['storage']['values']['submit'],
				$form_state['storage']['values']['form_build_id'],
				$form_state['storage']['values']['form_token'],
				$form_state['storage']['values']['form_id'],
				$form_state['storage']['values']['op']);

			//We have to recover the ID of the person we're editing, if we are
			//editing in the first place
			if (!empty($form_state['storage']['values']['person_id']))
			{
				$this->set_id($form_state['storage']['values']['person_id']);
			}

			//Begin the transaction
			$transaction = db\transaction();

			//Then handle our subforms.
			$this->process_children($form_state);

			//This should not cause a transaction commit in release builds.
			//This class needs to be extended to be meaningful in release!
			$transaction->commit(
				PERSON_ADD_LOG_MESSAGE,
				PERSON_PERSONAL_PARTICULARS_TAG,
				DEBUG ? MODULE_NAME : null,
				array('%person' => $form_state['storage']['values']['name'])
			);
		}

		/**
		 * Sets the ID of this form object
		 * 
		 * @param integer $person_id The database ID of the person.
		 */
		public function set_id($person_id)
		{
			if ($person_id === null)
			{
				throw new \InvalidArgumentException('$person_id');
			}

			$this->person_id = $person_id;
		}

		/**
		 * Gets the ID of this form object. If this returns null, the object
		 * does not yet exist in the database.
		 *
		 * @return int
		 */
		public function get_id()
		{
			return $this->person_id;
		}
	}

	class PersonalParticularsForm extends Form
	{
		/**
		 * True if the form should be disabled.
		 * @var boolean
		 */
		private $disabled;

		/**
		 * Constructor.
		 *
		 * @param boolean $disabled True if the form should be read-only.
		 */
		public function __construct($disabled = false)
		{
			parent::__construct();
			$this->disabled = $disabled;
		}

		/**
		 * Builds the Personal Particulars form.
		 *
		 * @param array $form_state The current Drupal form state
		 */
		protected function do_build(&$form_state)
		{
			$values = empty($form_state['storage']['values']) ? null :
				$form_state['storage']['values'];

			//Fill from database if we our parent form has a person to show.
			if ($this->get_parent()->get_id() !== null)
			{
				if (empty($form_state['input']))
				{
					$this->load($this->get_parent()->get_id(), $values);
				}
				else if (empty($form_state['input']['photograph']))
				{
					$this->load_photograph($this->get_parent()->get_id(), $values);
				}
			}

			//Hook the age calculation code
			$form['#attached']['js'] = array(
				'//cdnjs.cloudflare.com/ajax/libs/moment.js/2.0.0/moment.min.js',
				drupal_get_path('module', MODULE_NAME) . '/cvwobase_d7_person.js'
			);

			$form['nric_fin'] = array(
				'#type'				 => 'textfield',
				'#title'			 => t('NRIC/FIN'),
				'#size'				 => 25,
				'#maxlength'		 => 32,
				'#default_value'	 => isset($values['nric_fin']) ? $values['nric_fin'] : null,
				'#disabled'			 => $this->disabled
			);

			$form['nric_colour'] = array(
				'#type'			 => 'cvwocodem2_select',
				'#title'		 => t('NRIC Colour'),
				'#module'		 => MODULE_NAME,
				'#question'		 => PERSONAL_PARTICULARS_NRIC_COLOUR_QUESTION,
				'#default_value' => isset($values['nric_colour']) ? $values['nric_colour'] : null,
				'#disabled'		 => $this->disabled
			);

			$form['gender'] = array(
				'#type'			 => 'cvwocodem2_select',
				'#title'		 => t('Gender'),
				'#module'		 => MODULE_NAME,
				'#question'		 => PERSONAL_PARTICULARS_GENDER_QUESTION,
				'#default_value' => isset($values['gender']) ? $values['gender'] : null,
				'#disabled'		 => $this->disabled
			);

			//Try loading the image from database.
			//We will try the database first, and if that fails, we'll fall back
			//to the default image.
			$scaled_width		 = 0;
			$scaled_height		 = 0;
			$stored_image		 = empty($values['photograph']) ?
				null : file_load($values['photograph']);
			if (!empty($stored_image))
			{
				$image_size		 = getimagesize(drupal_realpath($stored_image->uri));
				$image_path		 = file_uri_scheme($stored_image->uri) === 'temporary' ?
					file_create_url($stored_image->uri) :
					url(sprintf(namespace\MODULE_NAME . '/person/%d/photo',
						$this->get_parent()->get_id()));
				$scaled_size	 = rescale_image($image_size, array(165, 200));
				$scaled_width	 = $scaled_size[0];
				$scaled_height	 = $scaled_size[1];
			}
			if (empty($image_path))
			{
				$image_path		 = $GLOBALS['base_path'] .
					drupal_get_path('module', namespace\MODULE_NAME) .
					namespace\PERSONAL_PARTICULARS_BLANK_PHOTO_PATH;
				$scaled_width	 = 165;
				$scaled_height	 = 200;
			}

			$form['photo'] = array(
				'#type'				 => 'managed_file',
				'#upload_location'	 => 'temporary://',
				'#theme'			 => 'form_image_upload',
				'#default_value'	 => $image_path,
				'#width'			 => $scaled_width,
				'#height'			 => $scaled_height,
				'#disabled'			 => $this->disabled,
				'#upload_validators' => array(
					'file_validate_is_image' => array()
				)
			);

			$form['salutation'] = array(
				'#type'			 => 'cvwocodem2_select',
				'#title'		 => t('Salutation'),
				'#module'		 => MODULE_NAME,
				'#question'		 => PERSONAL_PARTICULARS_SALUTATION_QUESTION,
				'#default_value' => isset($values['salutation']) ? $values['salutation'] : null,
				'#disabled'		 => $this->disabled
			);

			$form['name'] = array(
				'#type'			 => 'textfield',
				'#title'		 => t('Name'),
				'#size'			 => 30,
				'#maxlength'	 => 255,
				'#default_value' => isset($values['name']) ? $values['name'] : null,
				'#disabled'		 => $this->disabled
			);

			$form['traditional_name'] = array(
				'#type'			 => 'textfield',
				'#title'		 => t('Traditional Name'),
				'#size'			 => 15,
				'#maxlength'	 => 255,
				'#default_value' => isset($values['traditional_name']) ? $values['traditional_name'] : null,
				'#disabled'		 => $this->disabled
			);

			$form['marital_status'] = array(
				'#type'			 => 'cvwocodem2_select',
				'#title'		 => t('Marital Status'),
				'#module'		 => MODULE_NAME,
				'#question'		 => PERSONAL_PARTICULARS_MARITAL_STATUS_QUESTION,
				'#default_value' => isset($values['marital_status']) ? $values['marital_status'] : null,
				'#disabled'		 => $this->disabled
			);

			$form['nationality'] = array(
				'#type'			 => 'cvwocodem2_select',
				'#title'		 => t('Nationality'),
				'#module'		 => MODULE_NAME,
				'#question'		 => PERSONAL_PARTICULARS_NATIONALITY_QUESTION,
				'#default_value' => isset($values['nationality']) ? $values['nationality'] : null,
				'#disabled'		 => $this->disabled
			);

			$form['place_of_birth'] = array(
				'#type'			 => 'cvwocodem2_select',
				'#title'		 => t('Place of Birth'),
				'#module'		 => MODULE_NAME,
				'#question'		 => PERSONAL_PARTICULARS_PLACE_OF_BIRTH_QUESTION,
				'#default_value' => isset($values['place_of_birth']) ? $values['place_of_birth'] : null,
				'#disabled'		 => $this->disabled
			);

			$form['date_of_birth'] = array(
				'#type'					 => 'container',
				'#suffix'				 => "\n",
				'date_of_birth'			 => array(
					'#type'					 => 'date_popup',
					'#title'				 => t('Date of Birth'),
					'#date_format'			 => \CVWO\Base\DATE_FORMAT_DATE,
					'#date_label_position'	 => 'none',
					'#default_value'		 => isset($values['date_of_birth']) ?
						$values['date_of_birth'] : null,
					'#disabled'				 => $this->disabled,
					'#date_year_range'		 => '-100:0',
					'#partial_date'			 => true,
					'#size'					 => 8
				),
				'age'					 => array(
					'#type'					 => 'item',
					'#title'				 => t('Age'),
					'#title_display'		 => 'none',
					'#markup'				 => isset($values['date_of_birth']) ?
						t('(@age y.o.)',
							array('@age' => calculate_age(new \DateTime($values['date_of_birth'])))
						) : '',
					'#id'					 => 'edit-age'
				)
			);

			$form['religion'] = array(
				'#type'			 => 'cvwocodem2_select',
				'#title'		 => t('Religion'),
				'#module'		 => MODULE_NAME,
				'#question'		 => PERSONAL_PARTICULARS_RELIGION_QUESTION,
				'#default_value' => isset($values['religion']) ? $values['religion'] : null,
				'#disabled'		 => $this->disabled
			);

			$form['race'] = array(
				'#type'			 => 'cvwocodem2_select',
				'#title'		 => t('Race'),
				'#module'		 => MODULE_NAME,
				'#question'		 => PERSONAL_PARTICULARS_RACE_QUESTION,
				'#default_value' => isset($values['race']) ? $values['race'] : null,
				'#disabled'		 => $this->disabled
			);

			$form['ethnicity']	 = array(
				'#type'			 => 'cvwocodem2_select',
				'#title'		 => t('Ethnicity'),
				'#module'		 => MODULE_NAME,
				'#question'		 => PERSONAL_PARTICULARS_ETHNICITY_QUESTION,
				'#default_value' => isset($values['ethnicity']) ? $values['ethnicity'] : null,
				'#disabled'		 => $this->disabled
			);

			$form['highest_edu_level']	 = array(
				'#type'			 => 'cvwocodem2_select',
				'#title'		 => t('Highest Education'),
				'#module'		 => MODULE_NAME,
				'#question'		 => PERSONAL_PARTICULARS_HIGHEST_EDUCATION_LEVEL_QUESTION,
				'#default_value' => isset($values['highest_edu_level']) ? $values['highest_edu_level'] : null,
				'#disabled'		 => $this->disabled
			);

			$form['occupation']	 = array(
				'#type'			 => 'cvwocodem2_autocomplete',
				'#title'		 => t('Occupation'),
				'#module'		 => MODULE_NAME,
				'#question'		 => PERSONAL_PARTICULARS_OCCUPATION_QUESTION,
				'#default_value' => isset($values['occupation']) ? $values['occupation'] : null,
				'#disabled'		 => $this->disabled
			);

			$form['occupation_status']	 = array(
				'#type'			 => 'cvwocodem2_select',
				'#title'		 => t('Occupation Status'),
				'#module'		 => MODULE_NAME,
				'#question'		 => PERSONAL_PARTICULARS_OCCUPATION_STATUS_QUESTION,
				'#default_value' => isset($values['occupation_status']) ? $values['occupation_status'] : null,
				'#disabled'		 => $this->disabled
			);

			$this->entries = array_merge_recursive($this->entries, $form);
		}
		
		protected function do_validate(&$form, &$form_state)
		{
			if (!empty($form_state['values']['nric_fin']))
			{
				//Make sure you don't have duplicate nric/fin when adding new Person
				$parent_id = $this->get_parent()->get_id();
				if (Person::nric_exists($form_state['values']['nric_fin']) &&
					Person::from_nric($form_state['values']['nric_fin'])->get_id() !== $parent_id) {
					form_set_error('person', t('Another person with the same NRIC/FIN '
						. 'already exists.'));
				}
				
				//And make sure the NRIC/FIN entered is valid.
				if (!Person::validate_nric($form_state['values']['nric_fin']))
				{
					form_set_error('nric_fin', t('The entered NRIC/FIN is invalid.'));
				}
			}
			
			parent::do_validate($form, $form_state);
		}

		protected function do_process(&$form_state)
		{
			$id = $this->get_parent()->get_id();
			if (empty($id))
			{
				$id = $this->add($form_state);
			}
			else
			{
				$this->update($form_state, $id);
			}
		}

		/**
		 * Fills the form from the database.
		 * 
		 * @param integer $person_id The person ID to fill the data from.
		 * @param array $values The form state's values array.
		 */
		private function load($person_id, &$values)
		{
			$fields = array('nric_fin', 'nric_colour', 'gender', 'salutation',
				'name', 'traditional_name', 'marital_status', 'nationality',
				'place_of_birth', 'date_of_birth', 'religion', 'race',
				'ethnicity', 'highest_edu_level', 'occupation',
				'occupation_status', 'photograph');

			$query = db\select(CVWO_PERSON_TABLE)
				->condition('person_id', $person_id);
			$query->fields(CVWO_PERSON_TABLE, $fields);
			$query = $query->execute();
			$row = $query->fetchAssoc();
			foreach ($fields as $field)
			{
				$values[$field] = $row[$field];
			}
		}

		/**
		 * Fills only the photograph from the database.
		 *
		 * This is necessary because multiple input forms cannot be cached. Also,
		 * we have a peculiar upload field, where not uploading anything equates
		 * keeping the current image. So we need to retrieve the current image
		 * even when we have a form submittal.
		 *
		 * @param integer $person_id The person ID to fill the data from
		 * @param array $values The form state's values array.
		 */
		private function load_photograph($person_id, &$values)
		{
			$row = db\select(CVWO_PERSON_TABLE)
				->condition('person_id', $person_id)
				->fields(CVWO_PERSON_TABLE, array('photograph'))
				->execute()
				->fetchAssoc();

			$values['photograph'] = $row['photograph'];
		}

		private function add(&$form_state)
		{
			$user_id = $GLOBALS['user']->uid;
			$values = $form_state['storage']['values'];

			//Copy the directly-mapped fields first.
			$fields			 = array('nric_fin', 'nric_colour', 'gender',
				'salutation', 'name', 'traditional_name', 'marital_status',
				'nationality', 'place_of_birth', 'date_of_birth', 'religion',
				'race', 'ethnicity', 'highest_edu_level', 'occupation',
				'occupation_status');
			$db_values		 = array();
			foreach ($fields as $field)
			{
				//Handle empty inputs as nulls.
				if (in_array($field, array('nric_fin', 'name', 'traditional_name',
					'nric_colour', 'gender', 'salutation', 'marital_status',
					'nationality', 'place_of_birth', 'date_of_birth',
					'religion', 'race', 'ethnicity', 'highest_edu_level',
					'occupation', 'occupation_status'), true) &&
					empty($values[$field]))
				{
					$db_values[] = null;
				}
				else
				{
					$db_values[] = &$values[$field];
				}
			}

			//Set the photograph of the person.
			$photo_file = file_load($values['photo']);
			if ($photo_file !== false)
			{
				//I'm using db_transaction here instead of db\transaction
				//because this transaction is for atomicity for the file move, and
				//is part of a larger operation.
				$transaction = db_transaction();
				$photo_file = file_move($photo_file, PERSONAL_PARTICULARS_PROFILE_PHOTO_PATH);
				if ($photo_file === false)
				{
					$transaction->rollback();
					throw new \Exception(t('Could not move temporary image to permanent storage.'));
				}

				$photo_file->status |= 1;
				file_save($photo_file);

				$fields[] = 'photograph';
				$db_values[] = $photo_file->fid;
				unset($transaction);
			}

			//Add the last-updated values.
			$fields[] = 'user_created';
			$db_values[] = $user_id;
			$fields[] = 'user_modified';
			$db_values[] = $user_id;
			$fields[] = 'date_created';
			$db_values[] = null;

			$person_id = db\insert(CVWO_PERSON_TABLE)
				->fields($fields, $db_values)
				->execute();
			$this->get_parent()->set_id($person_id);
			
			//Increment the refcount of this person's photo if a photo was uploaded.
			if (!empty($photo_file))
			{
				file_usage_add($photo_file, MODULE_NAME, 'person', $this->get_parent()->get_id());
			}
		}

		/**
		 * Updates the current user in the database.
		 * 
		 * @param array $form_state The Drupal form state.
		 * @param integer $id The ID of the person to update.
		 * @throws \Exception Error during profile photo handling.
		 */
		private function update(&$form_state, $id)
		{
			$user_id = $GLOBALS['user']->uid;
			$values = $form_state['storage']['values'];

			//Copy the directly-mapped fields first.
			$fields			 = array('nric_fin', 'nric_colour', 'gender',
				'salutation', 'name', 'traditional_name', 'marital_status',
				'nationality', 'place_of_birth', 'date_of_birth', 'religion',
				'race', 'ethnicity', 'highest_edu_level', 'occupation',
				'occupation_status');
			$fields			 = drupal_map_assoc($fields,
				function($key) use (&$values) {
					//Handle empty inputs as nulls.
					if (in_array($key, array('nric_fin', 'name', 'traditional_name',
						'nric_colour', 'gender', 'salutation', 'marital_status',
						'nationality', 'place_of_birth', 'date_of_birth',
						'religion', 'race', 'ethnicity', 'highest_edu_level',
						'occupation', 'occupation_status'), true) &&
						empty($values[$key]))
					{
						return null;
					}
					else
					{
						return $values[$key];
					}
				});
				
			if (!is_null($fields['date_of_birth']))
			{
				//We must be able to handle partial dates for the date-of-birth. Parse
				//the date.
				$date_of_birth = new \DateObject($fields['date_of_birth'],
					null, \DATE_FORMAT_DATETIME);

				//Then build our own database date. Can't use format, because partial dates
				//would give us garbage.
				$fields['date_of_birth'] = date_format_date($date_of_birth, 'custom',
					\CVWO\Base\DATE_FORMAT_DATABASE, null, true);
			}

			//Set the photograph of the person.
			$photo_file = !empty($values['photo']) ?
				file_load($values['photo']) : false;
			if ($photo_file !== false)
			{
				//I'm using db_transaction here instead of db\transaction
				//because this transaction is for atomicity for the file delete
				//and file move, and also because it is part of a larger operation
				//handled by db\transtion.
				$transaction = db_transaction();

				//Mark the old photo deleted first.
				$query = db\select(CVWO_PERSON_TABLE)
					->condition('person_id', $id);
				$query->addField(CVWO_PERSON_TABLE, 'photograph');
				$result = $query->execute();
				$row = $result->fetchAssoc();

				//Make the old photo file a temporary file so cron will clean it up.
				$old_photo_file = !empty($row['photograph']) ?
					file_load($row['photograph']) : false;
				if ($old_photo_file !== false)
				{
					file_usage_delete($old_photo_file, MODULE_NAME, 'person', $id);
					$old_photo_file->status &= ~1;
					file_save($old_photo_file);
				}

				$photo_file = file_move($photo_file, PERSONAL_PARTICULARS_PROFILE_PHOTO_PATH);
				if ($photo_file === false)
				{
					$transaction->rollback();
					throw new \Exception(t('Could not move temporary image to permanent storage.'));
				}

				$photo_file->status |= 1;
				file_save($photo_file);
				file_usage_add($photo_file, MODULE_NAME, 'person', $id);

				$fields['photograph'] = $photo_file->fid;
				unset($transaction);
			}

			//Add the last-updated values.
			$fields['user_modified'] = $user_id;

			db\update(CVWO_PERSON_TABLE)
				->fields($fields)
				->condition('person_id', $id)
				->execute();
		}
	}

	class PersonContactDetailsForm extends FormMultipleInput
	                               implements FormMultipleInputFormAdapter
	{
		/**
		 * Builds the Person - Contact Details form.
		 *
		 * @param array $form_state The current Drupal form state.
		 */
		protected function do_build(&$form_state)
		{
			$form = array();

			//Load the records from database
			if (empty($form_state['input']) && $this->get_parent() !== null &&
				$this->get_parent()->get_id() !== null)
			{
				$this->load($form_state, $this);
			}

			$form['contact_type'] = array(
				'#type'			 => 'cvwocodem2_select',
				'#module'		 => MODULE_NAME,
				'#question'		 => PERSONAL_PARTICULARS_CONTACT_TYPE_QUESTION,
				'#title'		 => t(PERSONAL_PARTICULARS_CONTACT_TYPE_QUESTION),
				'#title_display' => 'none',
				'#required'		 => true
			);

			$form['contact_value'] = array(
				'#type'			 => 'textfield',
				'#size'			 => 20,
				'#maxlength'	 => 100,
				'#title'		 => t('Contact Value'),
				'#title_display' => 'none'
			);

			$this->entries = array_merge_recursive($this->entries, $form);
			parent::do_build($form_state);
		}

		protected function do_validate(&$form, &$form_state)
		{
			return parent::do_validate($form, $form_state, $this);
		}

		protected function do_process(&$form_state)
		{
			$this->save($form_state, $this);
		}

		public function get_fields_mapping()
		{
			$result = array('contact_type', 'contact_value');
			return array_combine($result, $result);
		}

		public function get_current_entry_ids()
		{
			$person_id = $this->get_parent()->get_id();
			$query = db\select(CVWO_PERSON_CONTACT_TABLE)
				->condition('person_id', $person_id);
			$query->addField(CVWO_PERSON_CONTACT_TABLE, 'contact_id');
			$query = $query->execute();

			return $query->fetchCol(0);
		}

		public function get_first_entry(&$handle)
		{
			$person_id = $this->get_parent()->get_id();
			if (empty($person_id))
			{
				return false;
			}

			$query = db\select(CVWO_PERSON_CONTACT_TABLE)
				->condition('person_id', $person_id);
			$query->fields(CVWO_PERSON_CONTACT_TABLE,
				array_keys($this->get_fields_mapping()));
			$query = $query->execute();
			$handle = $query;

			return $handle->fetchAssoc();
		}

		public function get_next_entry(&$handle)
		{
			return $handle->fetchAssoc();
		}

		public function insert($entry)
		{
			$person_id = $this->get_parent()->get_id();
			$user_id = $GLOBALS['user']->uid;

			$entry['person_id'] = $person_id;
			$entry['user_created'] = $user_id;
			$entry['user_modified'] = $user_id;
			$entry['date_created'] = null;

			db\insert(CVWO_PERSON_CONTACT_TABLE)
				->fields($entry)
				->execute();
		}

		public function update($id, $entry)
		{
			$user_id = $GLOBALS['user']->uid;
			$entry['user_modified'] = $user_id;

			db\update(CVWO_PERSON_CONTACT_TABLE)
					->condition('contact_id', $id)
					->fields($entry)
					->execute();
		}

		public function delete($id)
		{
			db\delete(CVWO_PERSON_CONTACT_TABLE)
					->condition('contact_id', $id)
					->execute();
		}
	}

	class LinguisticAbilitiesForm extends FormMultipleInput
	                              implements FormMultipleInputFormAdapter
	{
		/**
		 * Builds the Person - Linguistic Abilities form.
		 *
		 * @param array $form The form to insert the linguistic abilities subform into
		 * @param array $form_state The current Drupal form state.
		 */
		protected function do_build(&$form_state)
		{
			$form = array();

			//Load the records from database
			if (empty($form_state['input']) && $this->get_parent()->get_id() !== null)
			{
				$this->load($form_state, $this);
			}

			$form['language'] = array(
				'#type'			 => 'cvwocodem2_select',
				'#module'		 => MODULE_NAME,
				'#question'		 => PERSONAL_PARTICULARS_LANGUAGE_QUESTION,
				'#title'		 => t(PERSONAL_PARTICULARS_LANGUAGE_QUESTION),
				'#title_display' => 'none',
				'#required'		 => true
			);

			$form['competency'] = array(
				'#type'			 => 'cvwocodem2_select',
				'#module'		 => MODULE_NAME,
				'#question'		 => PERSONAL_PARTICULARS_LANGUAGE_COMPETENCY_QUESTION,
				'#title'		 => t(PERSONAL_PARTICULARS_LANGUAGE_COMPETENCY_QUESTION),
				'#title_display' => 'none',
				'#required'		 => true
			);

			$form['proficiency'] = array(
				'#type'			 => 'cvwocodem2_select',
				'#module'		 => MODULE_NAME,
				'#question'		 => PERSONAL_PARTICULARS_LANGUAGE_PROFICIENCY_QUESTION,
				'#title'		 => t(PERSONAL_PARTICULARS_LANGUAGE_PROFICIENCY_QUESTION),
				'#title_display' => 'none'
			);

			$this->entries = array_merge_recursive($this->entries, $form);
			parent::do_build($form_state);
		}

		protected function do_validate(&$form, &$form_state)
		{
			return parent::do_validate($form, $form_state, $this);
		}

		protected function do_process(&$form_state)
		{
			$this->save($form_state, $this);
		}

		public function get_fields_mapping()
		{
			return array(
				'language'				 => 'language',
				'language_competency'	 => 'competency',
				'language_proficiency'	 => 'proficiency'
			);
		}

		public function get_current_entry_ids()
		{
			$person_id = $this->get_parent()->get_id();
			$query = db\select(CVWO_PERSON_LANGUAGE_TABLE)
				->condition('person_id', $person_id);
			$query->addField(CVWO_PERSON_LANGUAGE_TABLE, 'language_id');
			$query = $query->execute();

			return $query->fetchCol(0);
		}

		public function get_first_entry(&$handle)
		{
			$person_id = $this->get_parent()->get_id();
			if (empty($person_id))
			{
				return false;
			}

			$query = db\select(CVWO_PERSON_LANGUAGE_TABLE)
				->condition('person_id', $person_id);
			$query->fields(CVWO_PERSON_LANGUAGE_TABLE,
				array_keys($this->get_fields_mapping()));
			$query = $query->execute();
			$handle = $query;

			return $handle->fetchAssoc();
		}

		public function get_next_entry(&$handle)
		{
			return $handle->fetchAssoc();
		}

		public function insert($entry)
		{
			$person_id = $this->get_parent()->get_id();
			$user_id = $GLOBALS['user']->uid;
			
			$nullable_field = function(&$value) {
				if (empty($value))
				{
					$value = null;
				}
			};
			$nullable_field($entry['language_proficiency']);

			$entry['person_id'] = $person_id;
			$entry['user_created'] = $user_id;
			$entry['user_modified'] = $user_id;
			$entry['date_created'] = null;
			db\insert(CVWO_PERSON_LANGUAGE_TABLE)
				->fields($entry)
				->execute();
		}

		public function update($id, $entry)
		{
			$user_id = $GLOBALS['user']->uid;
			$entry['user_modified'] = $user_id;
			
			$nullable_field = function(&$value) {
				if (empty($value))
				{
					$value = null;
				}
			};
			$nullable_field($entry['language_proficiency']);

			db\update(CVWO_PERSON_LANGUAGE_TABLE)
					->condition('language_id', $id)
					->fields($entry)
					->execute();
		}

		public function delete($id)
		{
			db\delete(CVWO_PERSON_LANGUAGE_TABLE)
					->condition('language_id', $id)
					->execute();
		}
	}

	abstract class AddressFormBase extends FormMultipleInput
	                               implements FormMultipleInputFormAdapter
	{
		/**
		 * Builds the Person - Address form.
		 *
		 * @param array $form_state
		 */
		protected function do_build(&$form_state)
		{
			$form = array();

			//Load the records from database
			if (empty($form_state['input']) && $this->get_parent()->get_id() !== null)
			{
				$this->load($form_state, $this);
			}

			$form['blk_no'] = array(
				'#type'			 => 'textfield',
				'#size'			 => 4,
				'#maxlength'	 => 16,
				'#title'		 => t('Block')
			);

			$form['street_name'] = array(
				'#type'			 => 'textfield',
				'#size'			 => 30,
				'#maxlength'	 => 255,
				'#title'		 => t('Street')
			);

			$form['floor_no'] = array(
				'#type'			 => 'textfield',
				'#size'			 => 3,
				'#maxlength'	 => 5,
				'#title'		 => t('Floor'),
				'#field_prefix'	 => '#'
			);

			$form['unit_no'] = array(
				'#type'			 => 'textfield',
				'#size'			 => 5,
				'#maxlength'	 => 16,
				'#title'		 => t('Unit'),
				'#field_prefix'	 => '-',
			);

			$form['postal_code'] = array(
				'#type'			 => 'textfield',
				'#size'			 => 6,
				'#maxlength'	 => 6,
				'#title'		 => t('Postal Code'),
				'#field_prefix'	 => 'S(',
				'#field_suffix'	 => ')'
			);

			$form['building_name'] = array(
				'#type'			 => 'textfield',
				'#size'			 => 20,
				'#maxlength'	 => 255,
				'#title'		 => t('Building Name')
			);

			$form['flat_type'] = array(
				'#type'			 => 'cvwocodem2_select',
				'#module'		 => MODULE_NAME,
				'#question'		 => PERSONAL_PARTICULARS_FLAT_TYPE_QUESTION,
				'#title'		 => t('Flat Type')
			);

			$this->entries = array_merge_recursive($this->entries, $form);
			parent::do_build($form_state);
		}

		protected function do_validate(&$form, &$form_state)
		{
			return parent::do_validate($form, $form_state, $this);
		}

		protected function do_process(&$form_state)
		{
			$this->save($form_state, $this);
		}
	}
	
	class PersonAddressForm extends AddressFormBase
	{
		public function get_fields_mapping()
		{
			$result = array('blk_no', 'street_name',
				'building_name', 'floor_no', 'unit_no', 'postal_code',
				'flat_type');
			return array_combine($result, $result);
		}

		public function get_current_entry_ids()
		{
			$person_id = $this->get_parent()->get_id();
			$query = db\select(namespace\PERSONAL_PARTICULARS_PERSON_ADDRESS_TABLE)
				->condition('person_id', $person_id);
			$query->addField(namespace\PERSONAL_PARTICULARS_PERSON_ADDRESS_TABLE,
				'person_address_id');
			$query = $query->execute();

			return $query->fetchCol(0);
		}

		public function get_first_entry(&$handle)
		{
			$person_id = $this->get_parent()->get_id();
			if (empty($person_id))
			{
				return false;
			}

			$query = db\select(namespace\PERSONAL_PARTICULARS_PERSON_ADDRESS_TABLE, 'p')
				->condition('person_id', $person_id);
			$address_table_alias = $query->addJoin('INNER', CVWO_ADDRESS_TABLE,
				null, 'p.address_id = %alias.address_id');

			$query->fields($address_table_alias,
				array_keys($this->get_fields_mapping()));
			$query = $query->execute();
			$handle = $query;

			return $handle->fetchAssoc();
		}

		public function get_next_entry(&$handle)
		{
			return $handle->fetchAssoc();
		}

		public function insert($entry)
		{
			$person_id = $this->get_parent()->get_id();
			$user_id = $GLOBALS['user']->uid;

			//Start a transaction
			$transaction = db\transaction();

			//Insert the new address record first.
			$entry['user_created'] = $user_id;
			$entry['user_modified'] = $user_id;
			$entry['date_created'] = null;
			
			$nullable_field = function(&$value) {
				if (empty($value))
				{
					$value = null;
				}
			};
			$nullable_field($entry['flat_type']);
			
			$address_id = db\insert(CVWO_ADDRESS_TABLE)
				->fields($entry)
				->execute();

			$link = array();
			$link['person_id'] = $person_id;
			$link['address_id'] = $address_id;
			db\insert(namespace\PERSONAL_PARTICULARS_PERSON_ADDRESS_TABLE)
				->fields($link)
				->execute();

			//This transaction commit log should not be seen. It should be wrapped
			//in a larger transaction.
			assert(Database\Transaction::is_in_transaction());
			$transaction->commit('Inserted address record',
				'Personal Particulars', null);
		}

		public function update($id, $entry)
		{
			$user_id = $GLOBALS['user']->uid;
			$entry['user_modified'] = $user_id;

			//Start a transaction
			$transaction = db\transaction();

			//Get the address record ID
			$query = db\select(namespace\PERSONAL_PARTICULARS_PERSON_ADDRESS_TABLE)
				->condition('person_address_id', $id);
			$query->addField(namespace\PERSONAL_PARTICULARS_PERSON_ADDRESS_TABLE,
				'address_id');
			$query = $query->execute();
			$row = $query->fetchAssoc();

			$nullable_field = function(&$value) {
				if (empty($value))
				{
					$value = null;
				}
			};
			$nullable_field($entry['flat_type']);

			//Update the address record.
			db\update(CVWO_ADDRESS_TABLE)
					->condition('address_id', $row['address_id'])
					->fields($entry)
					->execute();

			//This transaction commit log should not be seen. It should be wrapped
			//in a larger transaction.
			assert(Database\Transaction::is_in_transaction());
			$transaction->commit('Updated address record',
				'Personal Particulars', null);
		}

		public function delete($id)
		{
			//Start a transaction
			$transaction = db\transaction();

			//Get the address record ID
			$query = db\select(namespace\PERSONAL_PARTICULARS_PERSON_ADDRESS_TABLE)
				->condition('person_address_id', $id);
			$query->addField(namespace\PERSONAL_PARTICULARS_PERSON_ADDRESS_TABLE,
				'address_id');
			$query = $query->execute();
			$row = $query->fetchAssoc();

			//Delete the link
			db\delete(namespace\PERSONAL_PARTICULARS_PERSON_ADDRESS_TABLE)
					->condition('person_address_id', $id)
					->execute();

			//Then delete from the addresses table
			db\delete(CVWO_ADDRESS_TABLE)
					->condition('address_id', $row['address_id'])
					->execute();

			//This transaction commit log should not be seen. It should be wrapped
			//in a larger transaction.
			assert(Database\Transaction::is_in_transaction());
			$transaction->commit('Deleted address record',
				'Personal particulars', null);
		}
	}
}
