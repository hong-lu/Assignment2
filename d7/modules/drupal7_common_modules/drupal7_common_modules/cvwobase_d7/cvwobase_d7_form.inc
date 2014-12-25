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
	 * A base class for all forms in our CVWO Projects.
	 *
	 * This is so that we can abstract all forms away to a bunch of interfaces.
	 * To use this class, construct your form by overriding @see do_build.
	 * Store the same form array that you would to Drupal in $this->entries.
	 * Take note that your class might be extended! Instead of assigning to
	 * $this->entries, you might want to do an @see array_merge instead.
	 *
	 * To handle form submissions, override @see do_process. You will receive
	 * the Drupal form state when your handler is called.
	 *
	 * To hook this up with Drupal, declare a normal procedural function to
	 * handle Drupal's hooks (Drupal cannot take static class functions as
	 * hook callbacks for now, unfortunately)
	 *
	 * If you want to implement a hook_form_FORM_ID_alter form hook (see
	 * https://api.drupal.org/api/drupal/modules!system!system.api.php/function/hook_form_FORM_ID_alter/7)
	 * declare your hook as normal. The form IDs will be the class name. For
	 * namespaced classes, hook using the unqualified class name (because \ cannot
	 * appear in PHP function identifiers). Because hooking is implemented
	 * internally by us, all hooks will run before Drupal's hooks are run.
	 * Furthermore, hooks are run from closest to the Form class to the most
	 * derived. The fully qualified class name is passed to the $form_id
	 * parameter of the form alter hook, so even if there are ambiguities
	 * caused by stripping the namespace, hooks can still distinguish forms.
	 * 
	 * Drupal might cache our forms. So this class implements Serializable so
	 * that we won't blow the memory limit when we serialize this object. If your
	 * code assumes that certain values are cached, remember to implement the
	 * interface. Furthermore, if your form uses AJAX, remember to declare your
	 * class in your module.info's files[] array, so that unserialize() can find
	 * your class.
	 *
	 * Finally, to allow hooks to interact with our form framework, the form
	 * instance is passed on as the 4th parameter. Drupal won't pass it, so
	 * hooks needing to handle both types of hook calls can determine who called
	 * by that.
	 */
	abstract class Form implements \Serializable
	{
		/**
		 * Constructor. Call in your derived class implementation!
		 */
		public function __construct()
		{
			//Attach the default forms CSS
			if (!isset($this->entries['#attached']))
			{
				$this->entries['#attached'] = array();
			}
		}

		/**
		 * @return string
		 */
		public function serialize()
		{
			return serialize(array(
				$this->parent
			));
		}

		/**
		 * @param string $data
		 */
		public function unserialize($data)
		{
			$value = unserialize($data);
			$this->parent = $value[0];
		}

		/**
		 * The main core of the class. Override this function to populate
		 * $this->entries with the form fields.
		 *
		 * @param array $form_state The current state of the form.
		 */
		protected abstract function do_build(&$form_state);

		/**
		 * Builds the form.
		 *
		 * @param array $form_state The current Drupal state of the form.
		 * @return array An array compatible with the Drupal Forms framework.
		 */
		public final function build(&$form_state)
		{
			//Make this idempotent
			if (!$this->built)
			{
				$this->do_build($form_state);
				$this->call_hooks($form_state);
				$this->built = true;
				$this->entries['#instance'] = &$this;
			}
			
			return $this->entries;
		}

		/**
		 * Resets this form, causing a rebuild when next required.
		 */
		protected final function reset()
		{
			$this->built = false;
			$this->entries = array();
		}

		/**
		 * Builds the form.
		 *
		 * @param array $form_state The current Drupal state of the form.
		 * @return array An array compatible with the Drupal Forms framework.
		 */
		protected final function rebuild(&$form_state)
		{
			$this->reset();
			return $this->build($form_state);
		}

		/**
		 * Chains two forms together.
		 *
		 * Forms can only have one parent; i.e. they cannot be children of two
		 * forms at the same time.
		 *
		 * @param Form $child The child form to add to this form. This will
		 *                    allow children forms to handle their own form
		 *                    entries when the parent form's do_process is
		 *                    called. Children forms' do_process will be called
		 *                    after the parent form's.
		 */
		public final function add_child(Form &$child)
		{
			//Each form can only have one parent
			assert(empty($child->parent));

			$child->parent = $this;
			array_push($this->children, $child);
		}

		/**
		 * Gets the parent of the current form, or null if this file is not
		 * a child of any other form.
		 *
		 * @return Form
		 */
		protected final function get_parent()
		{
			return $this->parent;
		}

		/**
		 * Handles form validation. By default, this does nothing.
		 *
		 * Remember to call superclass implementations.
		 *
		 * @param array $form The Drupal form. This can be modified by validators,
		 *                    HOWEVER care must be taken to ensure new elements
		 *                    are validated and not raw input.
		 * @param array $form_state The Drupal form state.
		 */
		protected function do_validate(&$form, &$form_state)
		{
			$this->validate_children($form, $form_state);
		}

		/**
		 * Validates the child forms.
		 *
		 * @param array $form The Drupal form. This can be modified by validators,
		 *                    HOWEVER care must be taken to ensure new elements
		 *                    are validated and not raw input.
		 * @param array $form_state The Drupal form state.
		 */
		private function validate_children(&$form, &$form_state)
		{
			foreach ($this->children as &$child)
			{
				$child->validate($form, $form_state);
			}
			unset($child);
		}

		/**
		 * Runs the validators for this form.
		 *
		 * @param array $form The Drupal form. This can be modified by validators,
		 *                    HOWEVER care must be taken to ensure new elements
		 *                    are validated and not raw input.
		 * @param array $form_state The Drupal form state.
		 */
		public function validate(&$form, &$form_state)
		{
			//Make sure we've built the form.
			$this->build($form_state);

			$this->do_validate($form, $form_state);
		}

		/**
		 * Handles a form submittal. Override this function and implement the
		 * logic for marshalling form data to the database and vice versa.
		 *
		 * Remember to call @see process_children, otherwise child forms
		 * will NOT get their data.
		 *
		 * @param array $form_state The current Drupal state of the form.
		 */
		protected abstract function do_process(&$form_state);

		/**
		 * Handles a form submittal.
		 *
		 * @param array $form_state The current Drupal state of the form.
		 */
		public function process(&$form_state)
		{
			//Make sure we've built the form.
			$this->build($form_state);

			$this->do_process($form_state);
		}

		/**
		 * Processes the form for all children.
		 *
		 * @param array $form_state The current Drupal state of the form.
		 */
		protected function process_children(&$form_state)
		{
			foreach ($this->children as &$child)
			{
				$child->process($form_state);
			}
			unset($child);
		}

		/**
		 * Executes all form hooks for this form.
		 *
		 * This calls the hooks from least derived to most derived class. See
		 * the class form notes for more information.
		 *
		 * @param array $form_state The current Drupal form state.
		 */
		private function call_hooks(&$form_state)
		{
			//Get the most derived class.
			$class_name = get_class($this);

			//Get all the loaded modules so we can check for hooks.
			$modules = module_list();

			//Find the inheritance chain to our class.
			$classes = array();
			for ( ; $class_name !== false; $class_name = get_parent_class($class_name))
			{
				$classes[] = $class_name;
			}

			//Flip the class list so we call the most generic hook first.
			$classes = array_reverse($classes);
			
			//Keep calling the class name hooks. We don't have the same notion
			//of form names as Drupal, because we use inheritance. So we will just
			//keep calling the classes in order.
			foreach ($classes as $class_name)
			{
				foreach ($modules as $module_name)
				{
					$function_name = sprintf('%s_form_%s_alter', $module_name,
						$class_name);
					if (function_exists($function_name))
					{
						$function_name($this->entries, $form_state, $class_name,
							$this);
					}

					//Try without \ for namespaced classes. We will just take
					//the class name without the namespace qualification.
					if (($last_qual = strrpos($class_name, '\\')) !== false)
					{
						$unqualified_class_name = substr($class_name, $last_qual + 1);
						$function_name = sprintf('%s_form_%s_alter', $module_name,
							$unqualified_class_name);
						if (function_exists($function_name))
						{
							$function_name($this->entries, $form_state,
								$class_name, $this);
						}
					}
				}
			}
		}

		/**
		 * Gets a copy of the children forms.
		 *
		 * @return array
		 */
		protected function &get_children()
		{
			return $this->children;
		}

		/**
		 * Helper to find the element with a given key.
		 * The key can be nested deeper than the first level. In other words, it
		 * recursively walks the array to find the given key and returns the value.
		 *
		 * This is useful to find the element given the $form_state['value'].
		 *
		 * If more than one array contains the key (possible!) the last one is
		 * returned.
		 *
		 * @param array $haystack The array to search. Possibly multidimensional.
		 * @param string $needle The key to look up.
		 * @param array $path The path to the key. If this is defined, the result
		 *                    can be used with drupal_array_get_nested_value as
		 *                    the key on the same array.
		 */
		public static function& array_find_key_nested(array &$haystack, $needle, array &$path = null)
		{
			$result = null;
			$original_path = $path;
			$descendent_path = array();

			$replace = false;
			foreach ($haystack as $key => &$value)
			{
				if ($key === $needle)
				{
					$descendent_path = array($key);
					$result = &$value;
				}

				if (is_array($value))
				{
					$extra_path = array();
					$temp = &self::array_find_key_nested($value, $needle, $extra_path);
					if ($temp !== null)
					{
						if (isset($path))
						{
							array_splice($extra_path, 0, $replace ? 1 : 0, $key);
						}

						$result = &$temp;
						$descendent_path = $extra_path;
					}
				}
			}

			$path = $path === null ? null : array_merge($original_path, $descendent_path);
			return $result;
		}

		/**
		 * Helper function for loading a one-to-many relationship.
		 * 
		 * @param \CVWO\Base\FormMultipleInputFormAdapter $adapter
		 * @return array The elements in the set satisfying the relationship.
		 */
		protected static function load_one_to_many(FormMultipleInputAdapter $adapter)
		{
			//Keep asking the adapter for the next entry.
			$handle;
			$result = array();
			for ($entry = $adapter->get_first_entry($handle); $entry !== false;
				$entry = $adapter->get_next_entry($handle))
			{
				$result[] = $entry;
			}

			return $result;
		}

		/**
		 * Helper function for saving a one-to-many relationship.
		 *
		 * @param array $values A set of values to store in the database. Each
		 *                      element will be passed to the $adapter.
		 * @param \CVWO\Base\FormMultipleInputAdapter $adapter
		 *     The adapter to use.
		 */
		protected static function save_one_to_many(array $values, FormMultipleInputAdapter $adapter)
		{
			//We only care about the values.
			$values = array_values($values);
			
			//See how many entries we have
			$count = count($values);
			
			//Query the number of elements we currently have. We will the set
			//entries which are already stored in the database with our new data.
			//If we have extra entries in the database when we're done, we'll
			//delete. If we do not have enough, we will insert.
			$existing_entry_ids = $adapter->get_current_entry_ids();

			//This block is commented out because MySQL does not allow us to
			//have a transient state in a transaction which the unique key constraint
			//is broken. See CC-95.
			if (false)
			{
			//Delete extras first so that we won't have conflicting keys
			reset($existing_entry_ids);
			for ($i = 0, $id = current($existing_entry_ids);
				$i < count($existing_entry_ids) - $count;
				++$i, $id = next($existing_entry_ids))
			{
				$adapter->delete($id);
			}

			for ($i = 0; $i < $count && $id !== false; ++$i, $id = next($existing_entry_ids))
			{
				$adapter->update($id, $values[$i]);
			}

			//Insert new entries
			for (; $i < $count; ++$i)
			{
				assert(current($existing_entry_ids) === false);
				$adapter->insert($values[$i]);
			}

			assert($i === $count);
			}
			else
			{
				//Delete all existing entries
				reset($existing_entry_ids);
				for ($i = 0, $id = current($existing_entry_ids);
					$i < count($existing_entry_ids);
					++$i, $id = next($existing_entry_ids))
				{
					$adapter->delete($id);
				}
				
				//Then insert all the entries
				for ($i = 0; $i < $count; ++$i)
				{
					$adapter->insert($values[$i]);
				}
			}
		}

		/**
		 * Stores whether we have built the form. Because our instances may
		 * be destroyed, we have to re-run the build code when handling submissions.
		 *
		 * @var boolean
		 */
		private $built = false;

		/**
		 * The Drupal form array represented by this object.
		 * @var array
		 */
		protected $entries = array();

		/**
		 * The parent of the current form.
		 * @var Form
		 */
		private $parent = null;
		
		/**
		 * The list of child forms belonging to this form.
		 * 
		 * @var array
		 */
		private $children = array();
	}

	/**
	 * Form adapter interface for dealing with one-to-many relations.
	 */
	interface FormMultipleInputAdapter
	{
		/**
		 * Gets the list of entry IDs currently in the data store.
		 *
		 * This is used by the Form class to determine the number of entries
		 * which is currently in the database so that existing entries are used
		 * to store new data, instead of deleting and inserting all over again.
		 *
		 * IDs from this list are passed when @see update or @see delete is
		 * called so that implementations of this interface can set or delete
		 * the appropriate entry.
		 *
		 * The values in this list must be unique, otherwise the same ID might
		 * be used to store two entrys when update or delete is called.
		 *
		 * @return array
		 */
		public function get_current_entry_ids();

		/**
		 * Gets the first entry from the form adapter. The first argument is an
		 * opaque (to the caller of this function) handle that will be passed to
		 * @see get_next_entry as the handle parameter as well.
		 *
		 * @param mixed $handle An opaque handle which will also be passed to
		 *                      @see get_next_entry
		 * @return mixed The first value in the set.
		 */
		public function get_first_entry(&$handle);

		/**
		 * Gets the following entry from the form adapter. The first argument is
		 * an opaque (to the caller of this function) handle that was passed to
		 * @see get_first_entry on the first call.
		 *
		 * @param mixed $handle An opaque handle which will also be passed to
		 *                      @see get_next_entry
		 * @return mixed The subsequent values in the set. Return false when
		 *               there are no more entries.
		 */
		public function get_next_entry(&$handle);

		/**
		 * This is a callback for when saving to persistent storage, a new entry
		 * is needed to store the given entry.
		 *
		 * @param mixed $entry The value to add to the database.
		 */
		public function insert($entry);

		/**
		 * This is a callback for when saving to persistent storage, the entry
		 * with the given ID be replaced with the new entry.
		 *
		 * @param mixed $id The ID, from the set returned earlier by
		 *                  @see get_current_entry_ids
		 * @param array $entry The value to set for the given ID.
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
}
