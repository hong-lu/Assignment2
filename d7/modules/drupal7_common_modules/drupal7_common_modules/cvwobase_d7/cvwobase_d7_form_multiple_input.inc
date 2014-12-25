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
	/**
	 * Extends the base Form implementation to accept multiple inputs per entry.
	 * This allows you to define a form, that contains a set of fields, and
	 * this basically transforms that such that the entire set of fields can be
	 * repeated as one entry. This mostly fits the use case where there is a
	 * one-to-many relationship between a pair of entities (the MultipleInput
	 * is used for the "many" side.)
	 *
	 * For example: one can define Personal Particulars form, and define it such
	 * that a person can have multiple contact numbers. Your contact numbers
	 * form should then extend FormMultipleInput, so the user can have one
	 * instance of the MultipleInput form for home telephone, one for mobile
	 * phone, etc.
	 *
	 * This class basically takes whatever form is built when @see do_process
	 * is called, and wraps sentinel values and other markers to allow users
	 * to add and delete entries on their computer. Processing the form is
	 * a bit more complicated than usual, because the form values are now all
	 * arrays, instead of primitives.
	 * 
	 * If you need to set certain records as non-removable, set #removable for
	 * that record to false. It defaults to true.
	 *
	 * This class will handle counting and all that for you. Just chill.
	 *
	 * Common problems:
	 * 1. Only first row appearing. Check that your columns map correctly.
	 *    All columns in the $form_state['values'] entry that this class
	 *    is aware of must have the same number of rows.
	 * 2. When handling multiple input form data, you're expecting an array
	 *    but you end up getting a scalar. Check that you did NOT assign any
	 *    #value for any field; if you must (e.g. hidden field), then assign
	 *    an array.
	 * 3. Remember to pass in your form adapter when doing validation. Otherwise,
	 *    there is no way to check that the input is consistent (i.e. same number
	 *    of rows per form field)
	 * 4. When accessing $form_state['values'] you expect an array, but the array
	 *    contents are scattered across all the HTML inputs. You didn't call
	 *    validate.
	 * 5. Radio buttons and checkboxes are strange in this class. Following the
	 *    multiple input paradigm, if you are using the FormMultipleInputAdapter
	 *    interface, the values provided will be true (or truthy) if they are checked.
	 *
	 *    If you directly access $form_state['values'], checkboxes will be the same,
	 *    i.e. an array of all checkboxes, with their return values, or (int)0 if
	 *    unchecked. Radio buttons it will just be the checked item's #return_value.
	 *    You'd most likely need to define #primary because #return_value is used
	 *    to deduce the values of such fields. You can manually set them, and they
	 *    will be respected, but isn't it just simpler to use #primary?
	 * 6. You can use this with the drupal_add_tabledrag component. However, when
	 *    you do, you cannot add new items into the table, otherwise the submitted
	 *    values is almost guaranteed to be wrong.
	 */
	abstract class FormMultipleInput extends Form
	{
		/**
		 * A random ID pointing to an element indicating where to insert new
		 * elements.
		 * 
		 * @var string
		 */
		private $sentinel;

		/**
		 * A list of all defined field names, without the indexer suffixes.
		 * @var array Of String.
		 */
		protected $field_names;

		/**
		 * A copy of all the template fields. This is an associative array, with key
		 * being a value found in $this->field_names; and value being the actual
		 * #type of the field.
		 * 
		 * @var array
		 */
		private $field_templates;

		/**
		 * Constructor
		 */
		public function __construct()
		{
			parent::__construct();
			$this->sentinel = uniqid();

			//Attach the multiple input client JavaScript.
			if (!isset($this->entries['#attached']))
			{
				$this->entries['#attached'] = array();
			}

			$this->entries['#attached'] = array_merge_recursive(
				$this->entries['#attached'],
				array(
					'js' => array(
						drupal_get_path('module', namespace\MODULE_NAME) .
							'/cvwobase_d7_form_multiple_input.js'
					),
					'css' => array(
						drupal_get_path('module', namespace\MODULE_NAME) .
							'/cvwobase_d7_form_multiple_input.css'
					)
				)
			);
		}

		/**
		 * We can't handle caching. So force a rebuild for us on unserialize.
		 */
		public function unserialize($data)
		{
			$this->reset();
			parent::unserialize($data);
		}

		/**
		 * Builds the form.
		 * 
		 * Call this after subclasses populate the form. All entries which exist
		 * when this function is called will be treated as one input.
		 *
		 * If you want to populate the form with values, populate the
		 * corresponding $form_state['values'] array, and this will set it as
		 * the default value for the field. @see load does this.
		 *
		 * @param array $form_state The Drupal form state.
		 */
		protected function do_build(&$form_state)
		{
			//Make all elements multiple-value.
			$this->field_names = self::get_repeat_fields($this->entries);
			$entries = &$this->entries;
			$this->field_templates = drupal_map_assoc($this->field_names,
				function($key) use (&$entries) {
					$result = \CVWO\Base\Form::array_find_key_nested($entries, $key);
					return $result;
				});
			unset($entries);

			//Wrap the existing entries into a group. This forms our template.
			$wrapper = $this->build_entry_wrapper();
			foreach ($this->field_names as $field)
			{
				$wrapper[$field] = $this->entries[$field];
				unset($this->entries[$field]);
			}

			//Add a key so that we can detect deleted elements. Added elements
			//have this set to blank. This works together with #primary.
			$wrapper['key'] = $this->build_key_element();

			//Add an indexer. Since we are submitting our form elements piecemeal,
			//we need to know the collection of suffixes which are used to carry
			//information. This works like 'key', but is internal to our class and
			//used to match submission IDs to the variable names.
			$wrapper['indexer'] = $this->build_indexer_element();

			//Add a Remove entry to our current item. We will have to change the
			//ID for this element later to match the first element generated by us.
			$wrapper['remove'] = $this->build_remove_button();
			$this->entries[] = $wrapper;

			//Count the number of entries we have been provided in the form state.
			//This is so that our generated client code is aware of the number of
			//elements and can generate the correct next ID.
			$count = 0;
			if (!empty($form_state['input'][$this->get_indexer_key_field_name()]))
			{
				$count = count($form_state['input'][$this->get_indexer_key_field_name()]);
			}
			else if (!empty($form_state['values']))
			{
				foreach ($this->field_names as $field)
				{
					$count = empty($form_state['values'][$field]) ?
						0 : count($form_state['values'][$field]);
					break;
				}
			}

			//Render a copy of the form field, so that we can add them dynamically.
			$duplicate_form = self::build_duplicate_form($this->entries, $form_state);
			$this->entries['#attached']['js'][sprintf(<<<'EOD'
jQuery(document).ready(function() {
	CVWO.Base.form_multiple_input.fields['%1$s'] =
		new CVWO.Base.form_multiple_input.field('%1$s', '%2$s', %3$d);
});
EOD
				, $this->sentinel,
				js_escape(drupal_render_children($duplicate_form)),
				max($count, 1))] = array(
					'type' => 'inline'
				);
			unset($duplicate_form);

			//Fix up the IDs in our wrapper element.
			$wrapper['key'] = $this->build_key_element(0);
			$wrapper['indexer'] = $this->build_indexer_element(0);
			$wrapper['remove'] = $this->build_remove_button(0);
			$wrapper = $this->build_entry_wrapper(0) + $wrapper;
			$this->entries[0] = $wrapper;

			if (!empty($this->field_names))
			{
				//Now we fill in the form fields with defaults. The first one will
				//be filled in using the form which was generated by our superclasses;
				//subsequent entries will be generated in the for loop which follows.
				$this->build_repeated_entries($form_state, $this->field_names, $count);
			}

			//Add the sentinel value so new entries appear correctly
			$this->entries['sentinel'] = array(
				'#type'			 => 'container',
				'#attributes'	 => array(
					'id'		 => $this->sentinel,
					'style'		 => array(
						'display: none'
					)
				)
			);

			//Then render the Add more button
			$this->entries['add_more_btn'] = array(
				'#type'			 => 'submit',
				'#name'			 => $this->get_add_button_id(),
				'#value'		 => 'Add more',
				'#attributes'	 => array(
					'onclick' =>
						sprintf('CVWO.Base.form_multiple_input.add(\'%s\'); return false',
							$this->sentinel),
					'class'		 => array(
						'form_multiple_input_entry'
					)
				)
			);
		}

		/**
		 * Gets the list of form fields from the $entries array of this class.
		 *
		 * Static because you need to populate it in do_build() before you can
		 * pass in $entres.
		 *
		 * @param array $entries
		 */
		private static function get_repeat_fields(array &$entries)
		{
			$field_names = array();
			foreach ($entries as $key => &$value)
			{
				//Ignore form attributes
				if (strpos($key, '#') === 0)
				{
					continue;
				}

				$field_names[] = $key;
			}
			//Release the reference which we used in the foreach.
			unset($value);

			return $field_names;
		}

		/**
		 * Builds a duplicate form for rendering.
		 *
		 * Pass in a raw Drupal $form, and this function will run the form through
		 * Drupal's form API, to get the form that can be rendered using
		 * drupal_render.
		 */
		protected static function build_duplicate_form($form, &$form_state)
		{
			$duplicate_form = array_slice($form, 0);
			$duplicate_form_state = array_slice($form_state, 0);
			$duplicate_form_state['no_cache'] = true;
			$duplicate_form_state_values =
				isset($form_state['values']) && is_array($form_state['values']) ?
					array_slice($form_state['values'], 0) : null;

			drupal_prepare_form(null, $duplicate_form, $duplicate_form_state);
			drupal_process_form(null, $duplicate_form, $duplicate_form_state);
			unset($duplicate_form['form_id']);
			unset($duplicate_form['form_build_id']);
			unset($duplicate_form['form_token']);
			unset($duplicate_form_state);
			if ($duplicate_form_state_values !== null)
			{
				$form_state['values'] =& $duplicate_form_state_values;
				unset($duplicate_form_state_values);
			}

			return $duplicate_form;
		}

		/**
		 * Builds the Key hidden form field so we can match deleted and added
		 * entries.
		 * 
		 * @param integer $id The ID to build the hidden form field for.
		 * @param mixed $key The key used to identify the form entry.
		 * @return array The Drupal form element generated.
		 */
		private function build_key_element($id = null, $key = null)
		{
			return array(
				'#type'				 => 'hidden',
				'#name'				 => $this->get_primary_key_field_name() . $id,
				'#attributes'		 => array(
					'id'				 => sprintf('%s__mi_key__%s',
						$this->sentinel, $id === null ? '' : intval($id))
				),
				'#default_value'	 => $key
			);
		}

		/**
		 * Builds the indexer hidden form field so we can identify which $_POST
		 * fields belong to us.
		 *
		 * @param integer $id The ID to build the hidden form field for.
		 * @return array The Drupal form element generated.
		 */
		private function build_indexer_element($id = null)
		{
			return array(
				'#type'				 => 'hidden',
				'#name'				 => $this->get_indexer_key_field_name() . '[]',
				'#default_value'	 => $id,
				'#attributes'		 => array(
					'class'			 => array(
						'form_multiple_input_indexer'
					)
				)
			);
		}

		/**
		 * Builds the remove button element for the given ID. If the ID is null,
		 * this builds a template.
		 *
		 * @param integer $id The ID to build the remove button for.
		 * @return array The Drupal form element generated.
		 */
		private function build_remove_button($id = null)
		{
			return array(
				'#type'			 => 'link',
				'#href'			 => '',
				'#id'			 => sprintf('%s__mi_r__%s',
					$this->sentinel, $id === null ? '' : intval($id)),
				'#title'		 => t('Remove'),
				'#attributes'	 => array(
					'class'		 => array(
						'form_multiple_input_entry'
					),
					'onclick'	 =>
						'CVWO.Base.form_multiple_input.remove_using_id(this.id); ' .
						'return false'
				)
			);
		}

		/**
		 * Builds the entry wrapper div for the given ID. If the ID is null,
		 * this builds a template.
		 *
		 * @param integer $id The ID to build the wrapper for.
		 * @return array An array of Drupal form elements generated. The first
		 *               element is the header of the wrapper; the second is
		 *               the footer of the wrapper.
		 */
		private function build_entry_wrapper($id = null)
		{
			return array(
				'#type'			 => 'container',
				'#id'			 => $this->build_entry_uniqid($id),
				'#attributes'	 => array(
					'class'		 => array(
						'form_multiple_input_entry'
					)
				)
			);
		}

		/**
		 * Builds the unique ID (for HTML elements) for the given ID. If the ID
		 * is null, this builds a template.
		 *
		 * @param integer $id The ID to build the unique ID for.
		 */
		protected final function build_entry_uniqid($id)
		{
			return sprintf('%s__mi__%s', $this->sentinel,
				$id === null ? '' : intval($id));
		}

		/**
		 * Gets the form ID of the Add More button for this form.
		 */
		private function get_add_button_id()
		{
			return $this->sentinel . 'add_more_btn';
		}

		/**
		 * Gets the $form_state['values'] prefix for all our primary keys.
		 */
		protected function get_primary_key_field_name()
		{
			return sprintf('%s__mi_key', str_replace('\\', '__', get_class($this)));
		}

		/**
		 * Gets the $form_state['values'] entry containing our indexer array.
		 * @return array
		 */
		protected function get_indexer_key_field_name()
		{
			return sprintf('%s__mi_indexer', str_replace('\\', '__', get_class($this)));
		}
		
		/**
		 * Gets the $form_state['values'] entry containing the list of removabilities
		 * for the records. This follows the order of the indexer.
		 * @return type
		 */
		protected function get_indexer_removable_field_name()
		{
			return sprintf('%s__mi_removable', str_replace('\\', '__', get_class($this)));
		}

		/**
		 * Builds the repeated entries in the form state based on the template
		 * passed to @see do_build.
		 *
		 * @param $count integer The number of entries defined in the form state
		 *                       values array.
		 */
		private function build_repeated_entries(&$form_state, &$field_names, $count)
		{
			//Helper functions
			$normalise_element = function(&$element, $value, $primary) use (&$form_state, &$values)
			{
				$value_field = $element['#type'] === 'item' ?
					'#markup' : ($element['#type'] === 'link' ?
						'#href' : '#default_value');
				if ($value_field === '#markup')
				{
					$value = nl2br($value);
				}

				$element[$value_field] = $value;

				//If this is a radio button we have to set the return value using #primary
				//if we are not already set in the first place.
				if ($element['#type'] === 'radio' && empty($element['#return_value']))
				{
					//Check that $value is not empty so that we know we are not
					//generating a template value. If $value is empty, we are templating
					//and so #primary won't be available.
					if (!empty($value) && empty($values) && empty($primary))
					{
						//Warn the user. That radio buttons do NOT work in multiple
						//input forms because of the HTTP spec. See class comments
						//above
						trigger_error('Radio buttons can be used with the multiple '
							. 'input form class only if #return_value or #primary '
							. 'is defined. See class documentation.', E_USER_WARNING);
					}

					$element['#return_value'] = $primary;

					//If our value is checked, we have to set the value to #return_value
					//so it will be checked. Implicit truthy value conversion. No choice.
					if (isset($element['#default_value']) && $element['#default_value'])
					{
						$element['#default_value'] = $primary;
					}
				}

				//If this is a link and we are given an empty value then
				//we have to remove the link.
				else if ($element['#type'] === 'link' && empty($value))
				{
					$element['#type'] = '';
				}
			};
			$duplicate_element = function(&$element)
			{
				$result = $element;

				return $result;
			};

			//Set the template.
			$this->entries[0]['remove0'] = &$this->entries[0]['remove'];
			$this->entries[0]['indexer0'] = &$this->entries[0]['indexer'];
			unset($this->entries[0]['key'], $this->entries[0]['remove'],
				$this->entries[0]['indexer']);

			$template = &$this->entries[0];
			unset($this->entries[0]);

			//Then pull every entry from our form state.
			$indexes = isset($form_state['input'][$this->get_indexer_key_field_name()]) ?
				$form_state['input'][$this->get_indexer_key_field_name()] :
				($count === 0 ? array() : array_keys(array_fill(0, $count, null)));
			$empty_input = empty($indexes);
			$offset = 0;

			//If we have an empty set of indexes, that means there are no entries
			//for us to display. Just display the templates.
			foreach ($indexes as $i)
			{
				$this->entries['group' . $offset] = $this->build_entry_wrapper($i);

				//Build the primary key field.
				$primary = $empty_input ? null :
						$this->get_form_value($form_state, '#primary', $offset);
				
				//Make our fields removable unless otherwise stated.
				$removable = $empty_input ? true :
						$this->get_form_value($form_state, '#removable', $offset);
				if (is_null($removable))
				{
					$removable = true;
				}

				$this->entries['group' . $offset][$this->get_primary_key_field_name() . $i] =
					$this->build_key_element($i, $primary);
				$this->entries['group' . $offset]['indexer' . $i] =
					$this->build_indexer_element($i);

				foreach ($field_names as $field)
				{
					//Radios are special. We make them adopt the name of the field
					//otherwise they will all be selectable.
					$field_name = $template[$field]['#type'] === 'radio' ? $field :
						$field . $i;

					$this->entries['group' . $offset][$field_name] =
						$duplicate_element($template[$field]);
					$normalise_element($this->entries['group' . $offset][$field_name],
						$empty_input ? null : $this->get_form_value($form_state, $field, $offset),
						$primary);

					if ($template[$field]['#type'] === 'radio')
					{
						//Alias so other classes get the expected field. Drupal will
						//process each form field exactly once.
						$this->entries['group' . $offset][$field . $i] =
							&$this->entries['group' . $offset][$field];
					}
				}

				if ($removable)
				{
					$this->entries['group' . $offset]['remove' . $i] = $this->build_remove_button($i);
				}
				else
				{
					//Store an entry here so that cached forms will know to ignore the remove field.
					$this->entries['group' . $offset][$this->get_indexer_removable_field_name() . $i]
						= array(
							'#type'		 => 'value',
							'#value'	 => 0
						);
					$this->entries['group' . $offset]['remove' . $i] = array('#type' => 'markup');
				}
				++$offset;
			}
		}

		/**
		 * Gets the $index'th element for $field, from either input or
		 * $form_state['values'].
		 * 
		 * @param array $form_state The Drupal form state.
		 * @param string $field The name of the field.
		 * @param integer $index The index of the field to get.
		 * @return The form value, or null if not defined.
		 */
		protected function get_form_value(&$form_state, $field, $index)
		{
			//Map our custom fields first.
			if ($field === '#primary')
			{
				$field = $this->get_primary_key_field_name();
			}
			else if ($field === '#removable')
			{
				$field = $this->get_indexer_removable_field_name();
			}

			//Try the input first.
			if (isset($form_state['input'][$this->get_indexer_key_field_name()]))
			{
				$indexers = $form_state['input'][$this->get_indexer_key_field_name()];
				if (isset($form_state['input'][$field . $indexers[$index]]))
				{
					return $form_state['input'][$field . $indexers[$index]];
				}
			}
			
			//Then the values.
			if (isset($form_state['values'][$field][$index]))
			{
				return $form_state['values'][$field][$index];
			}
			else
			{
				return null;
			}
		}

		/**
		 * Handles form validation. This implements the virtual function do_validate
		 * from Form.
		 *
		 * @param array $form The Drupal form. This can be modified by validators,
		 *                    HOWEVER care must be taken to ensure new elements
		 *                    are validated and not raw input.
		 * @param array $form_state The Drupal form state.
		 * @param FormMultipleInputAfapter $adapter
		 *   This hidden parameter will be used to validate all input fields.
		 *   It is compulsory. If you have no need for special form handling (unlikely)
		 *   then pass $this, and it will validate all fields that this form has.
		 */
		protected function do_validate(&$form, &$form_state)
		{
			//Keep track of our reentrance. This is because forms will call
			//subforms' validate etc. This is used to decide if we need to call
			//for a form rebuild if an error happens during validation.
			static $validation_depth = 0;
			$in_validation = $validation_depth++ !== 0;

			$adapter = func_get_arg(2);
			if (empty($adapter))
			{
				trigger_error('Pass in the $adapter element for validation. '
					. 'If no special handling is required, pass $this instead.');
			}
			else if ($adapter === $this && !($this instanceof FormMultipleInputFormAdapter))
			{
				//No special requirements. Just combine all fields together.
				$adapter = new FormMultipleInputDefaultFormAdapter($this,
					$this->field_names);
			}

			$this->validate_adapter($form, $form_state, $adapter);
			$result = parent::do_validate($form, $form_state);

			//Check if the form needs to be rebuilt (because validation failed).
			//We will only rebuild in the outermost validator.
			$errors = form_get_errors();
			if (!$in_validation && !empty($errors))
			{
				$this->validate_fail_handler($form, $form_state, $adapter);
			}

			return $result;
		}

		/**
		 * Handles form validation (proper). Makes sure all fields have the same
		 * number of elements.
		 *
		 * @param array $form_state The Drupal form state.
		 * @param \CVWO\Base\FormMultipleInputAdapter $adapter
		 *   This will be used to see which form fields need to be checked for
		 *   consistency.
		 */
		private function validate_adapter(&$form, &$form_state, FormMultipleInputAdapter $adapter)
		{
			$map = $adapter->get_fields_mapping();
			$map_keys = array_keys($map);
			$map_values = array_values($map);

			//Map the primary key as well.
			$map_values[] = $this->get_primary_key_field_name();
			
			//The keys and values of the field mapping must be unique
			assert(count(array_unique($map_keys)) === count($map_keys) &&
				count(array_unique($map_values)) === count($map_values));

			//Map all the entries into our array.
			$indexer = &$form_state['input'][$this->get_indexer_key_field_name()];
			if (empty($indexer))
			{
				//Nothing to see here.
				foreach ($map_values as $field)
				{
					$form_state['values'][$field] = array();
				}

				return;
			}

			$form_values = array();
			foreach ($map_values as $field)
			{
				//Ignore all mappings starting with #. We currently only define
				//#primary, but follow Drupal's convention.
				if ($field[0] === '#')
				{
					continue;
				}

				//See if we should ignore this field because it is a radio button
				$element = isset($this->field_templates[$field]) ? $this->field_templates[$field] :
					null;

				if (!empty($element['#type']) && $element['#type'] === 'radio')
				{
					//Radio buttons only allow one selection for a set. So in
					//a multiple input form, we only expect a scalar.
					continue;
				}

				$value = array();
				foreach ($indexer as $index)
				{
					if (!in_array($field . $index, array_keys($form_state['values']), true))
					{
						//There are two reasons. If we are cached, we could have missed
						//added a new element (the old cached form did not have it).
						//In that situation, rebuild.
						if (empty($form_state['rebuild_info']))
						{
							$this->reprocess_form($form, $form_state);
							return;
						}
						else
						{
							$path = array();
							$element = self::array_find_key_nested($form, $field . $indexer[0], $path);
							assert(is_null($element) || drupal_array_get_nested_value($form, $path) === $element);
							unset($duplicate_form);

							//Then set the error on the field.
							form_set_error(implode('][', $path), t('Inconsistent number of inputs for @element',
								array('@element' => !empty($element['#title']) ?
									$element['#title'] : $field)));
							$value[] = null;
						}
					}
					else
					{
						$value[] = $form_state['values'][$field . $index];
						unset($form_state['values'][$field . $index]);
					}
				}

				$form_values[$field] = $value;
				$value = array();
			}

			$form_state['values'] = array_merge($form_state['values'], $form_values);
		}

		private function reprocess_form(&$form, &$form_state)
		{
			$errors = drupal_get_messages('error'); 
			$form_errors = form_get_errors();
			if (!empty($errors['error']))
			{
				foreach ($errors['error'] as $key => &$message)
				{
					if (!in_array($message, $form_errors, true))
					{
						drupal_set_message($message, 'error');
					}
				}
			}
			form_clear_error();

			$this->rebuild_form($form, $form_state);
			$form_state['rebuild_info'] = &$form_state['build_info'];
			drupal_process_form($form_state['build_info']['form_id'], $form, $form_state);
			unset($form_state['rebuild_info']);
		}

		private function validate_fail_handler(&$form, &$form_state)
		{
			$errors = form_get_errors();
			if (empty($errors))
			{
				return;
			}

			$old_form = $form;
			$form_state['values'] = array();
			$form_state['must_validate'] = true;
			if (empty($form_state['rebuild_info']))
			{
				$this->reprocess_form($form, $form_state);
			}
		}

		private function rebuild_form(&$form, &$form_state)
		{
			//Deduce which form we need to rebuild.
			$build_info = null;
			if (isset($form_state['build_info']))
			{
				$build_info = &$form_state['build_info'];
			}
			else
			{
				$build_info = &$form_state['rebuild_info'];
			}

			$form_id = &$build_info['form_id'];
			$form = drupal_rebuild_form($form_id, $form_state);
		}

		/**
		 * Loads the data from the given form adapter to the form state.
		 *
		 * Specialisation of the @see \CVWO\Base\Form\load_one_to_many helper
		 * for multiple input forms.
		 *
		 * @param array $form_state The Drupal form state.
		 * @param FormMultipleInputFormAdapter $adapter The Form Adapter to marshal
		 *                                              form state to and from the
		 *                                              database.
		 */
		protected function load(&$form_state, FormMultipleInputFormAdapter $adapter)
		{
			$map = $adapter->get_fields_mapping();
			$map_keys = array_keys($map);
			$map_values = array_values($map);
			
			//The keys and values of the field mapping must be unique
			assert(count(array_unique($map_keys)) === count($map_keys) &&
				count(array_unique($map_values)) === count($map_values));

			//Keep asking the adapter for the next entry.
			$handle;
			$i = 0;
			for ($entry = $adapter->get_first_entry($handle); $entry !== false;
				$entry = $adapter->get_next_entry($handle), ++$i)
			{
				//Marshal it into the form state.
				foreach ($entry as $key => &$value)
				{
					//Only map keys which we have a relation defined.
					if (!isset($map[$key]))
					{
						continue;
					}

					$form_field = $map[$key];

					//See if we need to manipulate the form field name (for
					//one-way fields)
					if (substr($form_field, 0, 2) === '##')
					{
						$form_field = substr($form_field, 2);
					}

					//If we have a #primary map it to the correct form field
					if ($form_field === '#primary')
					{
						$form_field = $this->get_primary_key_field_name();
					}
					
					//If it's the removability field, also map it.
					else if ($form_field === '#removable')
					{
						$form_field = $this->get_indexer_removable_field_name();
					}

					//Make sure that we are handling an array. For efficiency,
					//run this only on the first loop (not sure how tight this loop
					//will get)
					if ($i === 0)
					{
						$form_state['values'][$form_field] = array();
					}

					$form_state['values'][$form_field][] = $value;
				}
			}
		}

		/**
		 * Saves the data from the given form state to the provided form adapter.
		 *
		 * Specialisation of the @see \CVWO\Base\Form\save_one_to_many helper
		 * for multiple input forms.
		 *
		 * @param array $form_state The Drupal form state.
		 * @param FormMultipleInputFormAdapter $adapter The Form Adapter to marshal
		 *                                              form state to and from the
		 *                                              database.
		 */
		protected function save(&$form_state, FormMultipleInputFormAdapter $adapter)
		{
			$map = $adapter->get_fields_mapping();
			$map_keys = array_keys($map);
			$map_values = array_values($map);
			
			//The keys and values of the field mapping must be unique
			assert(count(array_unique($map_keys)) === count($map_keys) &&
				count(array_unique($map_values)) === count($map_values));

			//Determine the number of elements. Because our validator has made
			//sure we have a consistent number of inputs, we can just check the
			//key element.
			$count = isset($form_state['input'][$this->get_indexer_key_field_name()]) &&
				is_array($form_state['input'][$this->get_indexer_key_field_name()]) ?
					count($form_state['input'][$this->get_indexer_key_field_name()]) : 0;

			//Decide which save function we will use. If a primary key was given
			//we will use that.
			if (in_array('#primary', $map_values, true))
			{
				$this->save_primary($form_state, $adapter, $map, $count);
			}
			else
			{
				$this->save_naive($form_state, $adapter, $map, $count);
			}
		}

		/**
		 * Builds a database record from the form entry, to be given to the form
		 * adapter as the $entry parameter.
		 *
		 * @param array $form_state
		 * @param array $map
		 * @param integer $index
		 * @return array
		 */
		private function build_entry(&$form_state, &$map, $index)
		{
			//Get the first entry's index as a suffix, so we can use to look up
			//elements.
			$suffix = $form_state['input'][$this->get_indexer_key_field_name()][$index];

			//Extract the same row from ALL the form fields and plow it into $entry.
			$entry = array();
			foreach ($map as $form_field => &$database_field)
			{
				if ($form_field[0] === '#')
				{
					//Two things can happen here.
					//#-prefixed fields, which are internal. e.g. #primary
					//Or, ##-prefixed fields, See
					//FormMultipleInputFormAdapter::get_fields_mapping
					//for explanation. This is a one-way field.
					continue;
				}

				//See if we need to do special processing for this field, because
				//of the type of HTML form element.
				$element = isset($this->field_templates[$form_field]) ?
					$this->field_templates[$form_field] : assert(false);

				if (!empty($element) && !empty($element['#type']))
				{
					if ($element['#type'] === 'radio')
					{
						//We have to find the actual element because we need to find the return value.
						$element = self::array_find_key_nested($this->entries, $form_field . $suffix);

						//Loose equality because HTML submits everything as strings.
						$entry[$database_field] =
							$element['#return_value'] == $form_state['values'][$form_field];
						continue;
					}
				}

				$entry[$database_field] = $form_state['values'][$form_field][$index];
			}

			return $entry;
		}

		private function save_naive(&$form_state, FormMultipleInputFormAdapter $adapter,
			&$map, $count)
		{
			$map = array_flip($map);
			
			//Query the number of elements we currently have. We will the set
			//entries which are already stored in the database with our new data.
			//If we have extra entries in the database when we're done, we'll
			//delete. If we do not have enough, we will insert.
			$existing_entry_ids = $adapter->get_current_entry_ids();

			reset($existing_entry_ids);
			for ($i = 0, $id = current($existing_entry_ids);
				$i < $count && $id !== false; ++$i, $id = next($existing_entry_ids))
			{
				$adapter->update($id, self::build_entry($form_state, $map, $i));
			}

			//Insert new entries
			for (; $i < $count; ++$i)
			{
				assert(current($existing_entry_ids) === false);
				$adapter->insert(self::build_entry($form_state, $map, $i));
			}

			//Delete extras
			for (; $id !== false; $id = next($existing_entry_ids))
			{
				assert($i === $count);
				$adapter->delete($id);
			}
		}

		private function save_primary(&$form_state, FormMultipleInputFormAdapter $adapter,
			&$map, $count)
		{
			$map = array_flip($map);

			//Get the primary keys submitted, as well as the existing entries.
			$primary = array();
			if (!empty($form_state['input'][$this->get_indexer_key_field_name()]))
			{
				$primary = $form_state['values'][$this->get_primary_key_field_name()];
			}
			$existing_entries = $adapter->get_current_entry_ids();
			if (empty($existing_entries))
			{
				$existing_entries = array();
			}

			//Compute the IDs requiring updating/deletion.
			$update_ids = array_intersect($primary, $existing_entries);
			$delete_ids = array_diff($existing_entries, $primary);

			//For primary keys and unique keys, delete entries first (so updates
			//and insertions can proceed, in case they might conflict)
			foreach ($delete_ids as $id)
			{
				$adapter->delete($id);
			}

			//Then update entries.
			for ($i = 0; $i < $count; ++$i)
			{
				if (in_array($primary[$i], $update_ids, true))
				{
					//We have to map the index offset to the primary key order
					//because if we have a Table Reorder component, the $form_state['values']
					//order is incorrect.
					$index = array_search($primary[$i],
						$form_state['values'][$this->get_primary_key_field_name()], true);
					$adapter->update($primary[$i], self::build_entry($form_state, $map, $index));
				}
			}
			
			//Finally insert. There's no efficient way to compute insert keys,
			//because all the keys evaluate empty() as true.
			for ($i = 0; $i < $count; ++$i)
			{
				//We cannot resolve order here because there are multiple possible new insertions.
				//So for such things, we CANNOT use reordering in the multiple input form.
				if (empty($primary[$i]))
				{
					$adapter->insert(self::build_entry($form_state, $map, $i));
				}
			}
		}
	}

	/**
	 * Extension to the Multiple Input Form class, to display the form as a
	 * table. This can be seen in the Activity Attendance and Registration
	 * forms in SAC/NL.
	 *
	 * Basically this is just a formatter for the theme_table function.
	 */
	abstract class FormMultipleInputTableForm extends FormMultipleInput
	{
		protected function do_build(&$form_state)
		{
			parent::do_build($form_state);

			//Nothing to do if no fields defined.
			if (empty($this->field_names))
			{
				return;
			}

			//Build the table display. This is a form, but the form fields don't
			//render by default (because we overrode the theme)
			$values = null;
			if (!empty($form_state['input']))
			{
				$values = &$form_state['input'];
			}
			else
			{
				$values = &$form_state['values'];
			}

			//Pick the key to use to determine the number of rows.
			$entry_count_key = (!empty($values['#primary']) ? '#primary' :
				$this->field_names[0]
			);
			$entry_count = empty($values[$entry_count_key]) ? 0 :
				count($values[$entry_count_key]);
			$indexers = !empty($form_state['input'][$this->get_indexer_key_field_name()]) ?
				$form_state['input'][$this->get_indexer_key_field_name()] :
				($entry_count === 0 ? array() : array_keys(array_fill(0, $entry_count, null)));
			
			//Then render the form
			$count = 0;
			foreach ($indexers as $i)
			{
				$row = array();

				foreach ($this->field_names as $field)
				{
					$row[$field] = array(
						'data' => &$this->entries['group' . $count][$field . $i]
					);
				}

				//Render the delete button
				$row[] = array(
					'data' => array(
						&$this->entries['group' . $count]['remove' . $i],
						&$this->entries['group' . $count][$this->get_primary_key_field_name() . $i],
						&$this->entries['group' . $count][$this->get_indexer_removable_field_name() . $i],
						&$this->entries['group' . $count]['indexer' . $i]
					)
				);

				$this->entries['#rows'][] = array(
					'id'			 => $this->build_entry_uniqid($i),
					'data'			 => $row
				);

				++$count;
			}

			//Regroup the entire multiple input form; The save button is not
			//maintained because it's currently very difficult to render one
			//table row alone.
			$this->entries = array(
				'form'	 => $this->entries
			);
		}
	}

	/**
	 * Form adapter interface for the multiple input form. Implement this class
	 * and use this together with the @see FormMultipleInput class to implement
	 * data insertion, updates and removal without needing to implement the
	 * differencing logic in your code.
	 */
	interface FormMultipleInputFormAdapter extends FormMultipleInputAdapter
	{
		/**
		 * Gets the fields mapping.
		 *
		 * Return an array with key-value pairs. Keys will be the database columns,
		 * values will be the form field which it maps to. Because both identify
		 * keys, the set of both values and keys must not have duplicate items.
		 *
		 * A special value #primary can be used as a value. This will be used
		 * to map entries identically to generated entries so that in the round-trip
		 * from persisntent storage to form and back, the exact elements which
		 * were added and deleted can be determined (and update/delete/add called
		 * on them). If #primary is not defined, then the form class will use a
		 * naive difference algorithm which might cause transient key constraint
		 * violations. It is important to note that implementations of this class
		 * must make sure that the keys in #primary form a proper subset of the
		 * keys retruend by get_current_entry_ids. Also, anything that empty()
		 * returns true for CANNOT be a #primary key.
		 *
		 * Values starting with 2 #'s will be treated as one-way values, which
		 * is only used for display and not for storage. The form state will be
		 * populated during the load cycle, but not passed as part of save.
		 *
		 * @return array
		 */
		public function get_fields_mapping();

		/**
		 * Gets the first entry from the form adapter. The first argument is an
		 * opaque (to the caller of this function) handle that will be passed to
		 * @see get_next_entry as the handle parameter as well.
		 *
		 * @param mixed $handle An opaque handle which will also be passed to
		 *                      @see get_next_entry
		 * @return array An associative array with keys being database field names,
		 *               as returned by @see get_fields_mapping, mapping to the
		 *               values which should be displayed on the form.
		 */
		public function get_first_entry(&$handle);

		/**
		 * Gets the following entry from the form adapter. The first argument is
		 * an opaque (to the caller of this function) handle that was passed to
		 * @see get_first_entry on the first call.
		 *
		 * @param mixed $handle An opaque handle which will also be passed to
		 *                      @see get_next_entry
		 * @return array An associative array with keys being database field names,
		 *               as returned by @see get_fields_mapping, mapping to the
		 *               values which should be displayed on the form.
		 *               Return false when there are no more entries.
		 */
		public function get_next_entry(&$handle);

		/**
		 * This is a callback for when saving to persistent storage, a new entry
		 * is needed to store the given entry.
		 *
		 * @param array $entry An associative array containing the database
		 *                     columns as keys, and the appropriate form value
		 *                     mapped to it for the current entry.
		 */
		public function insert($entry);

		/**
		 * This is a callback for when saving to persistent storage, the entry
		 * with the given ID be replaced with the new entry.
		 *
		 * @param mixed $id The ID, from the set returned earlier by
		 *                  @see get_current_entry_ids
		 * @param array $entry An associative array containing the database
		 *                     columns as keys, and the appropriate form value
		 *                     mapped to it for the current entry.
		 */
		public function update($id, $entry);

		/**
		 * This is a callback for when saving to persistent storage, the entry
		 * with the given ID needs to be removed because there are too many
		 * entries in storage than needed.
		 *
		 * @param mixed $id The ID, from the set returned earlier by
		 *                  @see get_current_entry_ids
		 */
		public function delete($id);
	}

	/**
	 * Default form adapter class. You should not need to use this, this is
	 * internal.
	 */
	class FormMultipleInputDefaultFormAdapter implements FormMultipleInputFormAdapter
	{
		/**
		 * The names of the fields of the form.
		 * @var array
		 */
		private $entries;

		/**
		 * Constructs the form validator.
		 *
		 * @param \CVWO\Base\FormMultipleInput $form The form to validate.
		 * @param array $entries The names of the fields of the form.
		 */
		public function __construct(FormMultipleInput $form, array $entries)
		{
			$this->entries = $entries;
		}

		public function get_fields_mapping()
		{
			return array_combine($this->entries, $this->entries);
		}

		public function get_current_entry_ids()
		{
		}

		public function get_first_entry(&$handle)
		{
			return false;
		}

		public function get_next_entry(&$handle)
		{
			return false;
		}

		public function insert($entry)
		{	
		}

		public function update($id, $entry)
		{
		}

		public function delete($id)
		{
		}
	}
}
