<?php
/**
* Copyright (c) 2010-2013
* Computing for Volunteer Welfare Organizations (CVWO)
* National University of Singapore
*
* Permission is hereby granted, free of charge, to any person obtainin
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject
* to the following conditions:
*
* 1. The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* 2. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
* NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
* BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
* ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
*/
// $Id:
class CVWOBaseSearch {
	private $serial_col = TRUE;
	private $form;
	private $search_name;
	private $disable_basic_search = FALSE;
	private $basic_search_element = array();
	private $basic_search_header = array();
	private $basic_search_query;
	private $basic_search_where_clause;
	private $basic_search_pagelimit = 10;

	private $hide_advanced_search = TRUE;
	private $adv_search_default_header = array();
	private $select_skip_array = array();
	private $join_column = '';
	private $default_where_clause = ""; // empty result message
	private $group_by = '';

	private $hidden_select_fields = array();
	private $filter_max_column = 4; //max number of columns in a row (filter)
	private $filter_list = array();
	private $codem_options = array();
	private $custom_filters = array();
	private $displayfield_max_column = 4; //max number of columns in a row (filter)
	private $display_fields = array();
	private $custom_columns = array(); // array of columns that need to be formatted customly (cannot be directly displayed)
	private $select_array = array();
	private $selected_tables = array();
	private $option_callback = null;
	private $form_render_callback = '';

	private $table_data = array();
	private $table_header = array();
	private $export_query = '';
	private $export_header = array();

	/** Constructor Function
	*
	* @param string $search_name Name of search
	* @param array_reference &$form Form array, passed by reference
	*/
	function CVWOBaseSearch($search_name, &$form) {
		$this->search_name = $search_name;
		$this->form = &$form;
	}

	/** Define Basic Search
	*
	* @param array $element The textfield from which the basic search is entered.
	*        This element must have explicit name of 'search_field'.
	* @param SelectQuery $query The query which the basic search would execute
	* @param array $header The headers of the table when basic search is executed
	* @param string $where The WHERE clause to use for all searches. The input will
	 *       be in the :str parameter.
	* @param int $pagelimit page limit
	*
	* originally in drupal6, $basic_search_query contains something like "WHERE ((LOWER(p.english_name) LIKE LOWER(\'%%%s%\'))"
	* replace with '((LOWER(p.english_name) LIKE LOWER(:str)'' in $where param
	*/
	function defineBasicSearch(&$element, &$query, $header, $where, $pagelimit = 10) {
		$this->basic_search_element   = &$element;
		$this->basic_search_query     = &$query;
		$this->basic_search_header    = $header;
		$this->basic_search_pagelimit = $pagelimit;
		$this->basic_search_where_clause = $where;
	}

	function basicSearchWithoutFields(&$query, $header, $pagelimit = 10) {
		$this->basic_search_query     = &$query;
		$this->basic_search_header    = $header;
		$this->basic_search_pagelimit = $pagelimit;
		$this->disable_basic_search = TRUE;
	}

	/** Define Advanced Search
	*
	* @param array $default_header
	* @param array $skip_fields An array where the keys are used to check if the
	*        column should be ignored
	* @param string $join_column The name of the table column which will be used to
	*        join two tables together. The same column name will be used on both
	*        sides of the join.
	* @param array $join_column An array of join column for different tables
	* @param string $default_where_clause The clause which will be added to the
	*        WHERE statement.
	* @param string $group_by The name of the column to group the results by.
	*/
	function defineAdvancedSearch($default_header, $skip_fields, $join_column, $default_where_clause = null, $group_by = '') {
		$this->adv_search_default_header = $default_header;
		$this->select_skip_array = $skip_fields;
		$this->join_column = $join_column;
		$this->default_where_clause = trim($default_where_clause);

		$this->group_by = $group_by;
		for ($i = 0; $i < count($default_header); $i++) {
			if (($dot_pos = strpos($default_header[$i]['field'], '.')) !== false) {
				$this->checkTableColumn(
					substr($default_header[$i]['field'], 0, $dot_pos),
					substr($default_header[$i]['field'], $dot_pos + 1)
				);
			} else if (isset($default_header[$i]['table'])) {
				$this->checkTableColumn(
					$default_header[$i]['field'],
					$default_header[$i]['table']
				);
			}
		}
	}

	function setAdvancedSearchQuery(&$query) {
		$this->advancedSearchQuery = &$query;
	}

	function setHiddenSelectFields($fields){
		$this->hidden_select_fields = $fields;
	}

	// Allow modification of default_where_clause
	function modifyDefaultWhereClause($clause){
			$this->default_where_clause .= 'AND ' . $clause;
	}

	/** Register Code Maintanence Filters
	*
	* @param array $filters Array in the format
	*
	* array(
	*   'db_table' => array(
	*     array('database_column', 'module_name', 'question_title'),
	*   ),
	* )
	*/
	function registerCodemFilter($filters){
		$this->filter_list = $filters;
	}
	function setSerialColumn($id){
		$this->serial_col = $id;
	}

	/** Register Code Display Fields
	*
	* @param array $filters Array in the format
	* array(
	*   'table_name'=> array(
	*     'table_field' => 'Title Text',
	*   );
	* );
	*/
	function registerDisplayFields( $fields ) {
		$this->display_fields = $fields;
	}

	/** Add Custom Filters
	*
	* Additional Filters that cannot be directly taken from code mainteanance
	* @param array $filters Array of form elements of custom filters
	*/
	function addCustomFilters($filter) {
		foreach($filter as $field => $element) {
			$this->custom_filters[] = $element;
		}
	}

	/**
	*Set the max number of columns in a row for filter
	*/
	function setFilterMaxColumn($column_size){
		$this->filter_max_column = $column_size;
	}

	/**
	*Set the max number of columns in a row for display fields
	*/
	function setDisplayFieldMaxColumn($column_size){
		$this->displayfield_max_column = $column_size;
	}

	function setOptionCallback($callback_function){
		$this->option_callback = $callback_function;
	}

	function setFormRenderCallback($callback_function) {
		$this->form_render_callback = $callback_function;
	}

	/** 
	 * Define Custom Columns
	 *
	 * @param array $columns This takes an associative array. The keys are the names
	 *        of the columns; the values can either be a callable which will be used
	 *        to display the contents of the field; it will be called with the database
	 *        row being fetched from the database (deprecated). The function prototype
	 *        is thus:
	 * 
	 *            function(array $row) {}
	 * 
	 *        OR it can be an array with two keys: query and display, with query
	 *        being called before an advanced search (so that expressions etc can
	 *        be added to the query) and display the same as the old callable described
	 *        above.
	 *
	 *        In query, the first argument is the database query.
	 *
	 *            function(SelectQuery $row) {}
	 *
	 *        In display, the prototype is the same as the deprecated one.
	 */
	function defineCustomColumns($columns) {
		foreach ($columns as $column => $callback) {
			$this->custom_columns[$column] = $callback;
		}
	}

	function setCustomColumn($column, $callback) {
		$this->custom_columns[$column] = $callback;
	}

	/** CVWO Base Form
	*
	* Contains 3 main parts. Basic search, advanced search, listing via table.
	* Sets up page for all other the table display
	*
	* @param unknown_type &$form_state Reference to the form state, including values
	* @param string $person_type Type of person which this module is in
	* @param array $additional_codem Fields that would be used in advanced search
	* @param array $fields Display fields on table
	*/
	function getForm(&$form_state = null){
		if (isset($form_state['triggering_element']['#id'])) {
			// User presses either search button 
			// Revert to the that search mode
			$get_query = array();
			switch ($form_state['triggering_element']['#id']) {
				case 'edit-basic-search-submit':
					$_SESSION['search_mode'] = 'basic';
					foreach ($this->basic_search_element as $key => $element) {
						if (substr($key, 0, 1) === '#') {
							//If we passed in a form element, ignore all # keys.
							continue;
						}

						$get_query[$key] = $form_state['values'][$key];
					}
				break;
				case 'adv_search':
					$_SESSION['search_mode'] = 'advanced';
				break;
			}
			$get_query['search_mode'] = $_SESSION['search_mode'];
			
			// Save the form_state inside $_SESSION so it will survive page redirects
			// When the user clicks on a pager link
			if($form_state['triggering_element']['#name']!='excel_export'){
				$_SESSION['values'] = $form_state['values'];
				drupal_goto(current_path(),
					array('alias' => true,
						  'query' => $get_query)); // TODO: Explain this weird shit
			}
		} else if (isset($_GET['search_mode']) && isset($_SESSION['values'])) {
			// User clicks on pager link
			$form_state['values'] = $_SESSION['values'];
		} else if (!isset($_GET['search_mode'])) {
			// If search_mode is not set, it means we have just entered the page from another module
			// Reset session
			$_SESSION['search_mode'] = 'basic';
			unset($_SESSION['values']);
		}

		$values = $this->getValues($form_state);

		$form_state['rebuild'] = true;
		$form_state['no_cache'] = true;

		// hidden field to keep track of search mode
		$this->form['search_mode'] = array(
      		'#type' => 'hidden',
      		'#attributes' => array('id' => 'search_mode'),
      		'#value' => $_SESSION['search_mode']
		);
		
		/*
		* Basic Search Form
		*/
		if (!$this->disable_basic_search){
			$this->form['basic_search']=$this->basic_search_element;
			if (!isset($this->form['basic_search']['#attributes']['class'])) {
				$this->form['basic_search']['#attributes']['class'] = array();
			}
			$this->form['basic_search']['#attributes']['class'][] = 'no-print';
			if (isset($values['search_field'])) {
				$this->form['basic_search']['search_field']['#default_value'] = $values['search_field'];
			}
			$this->form['basic_search']['basic_search_submit'] = array(
	        	'#type' => 'button',
    	   		'#name'=>'basic_search_submit',
        		'#value' => t('Search')
			);
		}

		/*
		* Advanced Search Form
		*/
		$this->form['adv_search'] = array(
      		'#type' => 'fieldset',
      		'#title' => t('Advanced Search'),
      		'#collapsible' => TRUE,
      		'#collapsed' => $this->hide_advanced_search,
			'#attributes' => array(
				'class' => array(
					'no-print'
				)
			)
		);

		/*
		* Filters
		*/
		$this->form['adv_search']['filter'] = array(
      		'#type' => 'fieldset',
      		'#title' => t('Search Filters'),
      		'#collapsible' => TRUE,
      		'#collapsed' => FALSE,
		);

		/*
		* Custom Filter
		*/
		for($i = 0; $i< count($this->custom_filters); $i++) {
			$this->form['adv_search']['filter']['customFilter_'.$i] = $this->custom_filters[$i];

			if (isset($values['customFilter_'.$i]) && $values['customFilter_'.$i]){
				if(is_array($values['customFilter_'.$i]) &&
					isset($values['customFilter_'.$i]['date']) &&
					strlen($values['customFilter_'.$i]['date'])>0 ) {
					list($d1, $m1, $y1) = explode('-', $values['customFilter_'.$i]['date']);
					$new = implode('-', array($y1, $m1, $d1)).' 00:00:00';
					$this->form['adv_search']['filter']['customFilter_'.$i]['#default_value'] = $new;
				} else {
                	$this->form['adv_search']['filter']['customFilter_'.$i]['#default_value'] = $values['customFilter_'.$i];
				}

            }
        }

		foreach($this->filter_list as $db_table => $codems) {
			for($i = 0; $i< count($codems); $i++) {
				$options =  cvwocodem2_getoptions2($codems[$i][1], $codems[$i][2], 0, TRUE);
				$field = $codems[$i][0];
				$this->form['adv_search']['filter'][$field] = array(
				'#type' => 'select',
				'#title' => $codems[$i][2],
          		'#options' => $options,
				);

				if(isset($values[$field])){
					$this->form['adv_search']['filter'][$field]['#default_value'] =  $values[$field];
				}

				$this->codem_options[$codems[$i][0]] = $options;
			}
		}

		/*
		* Display Fields
		*/
		if ($this->display_fields != array()){
			$this->form['adv_search']['display'] = array(
        	'#type' => 'fieldset',
        	'#title'=>'Display Field',
        	'#collapsible'=> true,
        	'#collased'=> false,
			);

		foreach($this->display_fields as $table => $fields)
			foreach($fields as $field=> $name) {
				$this->form['adv_search']['display'][$field.'-display'] = array(
            	'#type' => 'checkbox',
            	'#title'=> $name,
				);
				if(isset($values[$field.'-display'])){
					$this->form['adv_search']['display'][$field.'-display']['#default_value']=$values[$field.'-display'];
				}
			}
			$fields = &$this->form['adv_search']['display'];
			uasort($fields, array($this, 'sortDisplayFields'));
		}

		$options = cvwocodem2_getoptions(\CVWO\Base\MODULE_NAME, PAGELIMIT_CVWOBASE, 0, false);

		$this->form['adv_search']['pagelimit'] = array(
      		'#type' => 'select',
      		'#options' => $options,
      		'#default_value' => $values['pagelimit'],
      		'#prefix'=>'<table><tbody style="border: none"><tr><td width="70px">Page Limit</td><td width="80px">',
      		'#suffix'=>'</td>'
      );
      $this->form['adv_search']['adv_search_submit'] = array(
      		'#type' => 'button',
      		'#value' => t('Advanced Search'),
      		'#name' => 'adv_search',
      		'#id' => 'adv_search',
      		// '#attributes' => array('onclick' => 'document.getElementById("search_mode").value="advanced"'),
      		'#prefix'=>'<td>',
      		'#suffix'=>'</td></tr></tbody></table>'
      );
      $this->form['output'] = $this->getOutput($values);
     if(isset($form_state['triggering_element']['#name'])&&$form_state['triggering_element']['#name'] == 'excel_export'){
      	$this->exportToExcel();
   	}
      if(strlen($this->form_render_callback)>1) {
      	$this->form = call_user_func($this->form_render_callback, $this->form); //$this->form must be passed by reference
      }

	  //Attach our CSS
	  if (!isset($this->form['#attached']['css']))
	  {
		  $this->form['#attached']['css'] = array();
	  }
	  $this->form['#attached']['css'][] = drupal_get_path('module', \CVWO\Base\MODULE_NAME) . '/cvwobase_d7_search.css';
      return $this->form;
	}

	/**
	 * Gets the $values for the search query.
	 */
	private function getValues(&$form_state)
	{
		$values = (!isset($_SESSION['values']) ? $_GET : $_SESSION['values']);

		if (isset($this->form['basic_search']['condition_field']) && !isset($values['condition_field'])) {
			$values['condition_field'] = 0;
		}

		if (isset($this->basic_search_element['condition_field'])) {
			$values['condition_field'] = $this->basic_search_element['condition_field']['#default_value'];
		}

		if (!isset($form_state['values']['pagelimit'])){
			$values['pagelimit'] = 100;
		} else {
			$values['pagelimit'] = $form_state['values']['pagelimit'];
		}

		return $values;
	}

	/**
	 * Gets the query which will return all results which is displayed. This can
	 * be used to operate on the result set that the user specified.
	 *
	 * If not all the results fit in the page, this will still return them.
	 */
	public function getQuery(&$form_state)
	{
		if ($_SESSION['search_mode'] == 'basic') {
		 	return $this->getBasicSearchQuery($form_state);
		} else {
			return $this->getAdvancedSearchQuery($form_state);
		}
	}

	/** getOutput
	*
	* Decides which search output to call based on search mode
	*
	* @param array $values Reference to form
	* @param array $form_state Reference to form
	*/

	private function getOutput(&$values){
		if($_SESSION['search_mode'] == 'basic') {
		 	return $this->getBasicSearchOutput($values);
		} else {
			return $this->getAdvancedSearchOutput($values);
		}
	}
	
	private function sortDisplayFields($a, $b) {
		if(isset($a['#title']) && isset($b['#title']))
			return strcasecmp($a['#title'], $b['#title']);
		return -1;
	}

	private function removedynamic($header) {
		//Remove all the fields which are custom (sorting by them makes no sense
		//because the DB will sort them by the internal representation, not the
		//display representation)
		foreach ($header as &$column) {
			if (array_key_exists($column['field'], $this->custom_columns) &&
				
				//It must be the newfangled array thing. Old ones with just
				//lambdas assume it's sortable.
				is_array($this->custom_columns[$column['field']]) &&
				
				//And if it's using the array, make sure that either we do not define
				//the sortable attribute, or that it is set to true
				!(!isset($this->custom_columns[$column['field']]['sortable']) ||
					is_null($this->custom_columns[$column['field']]['sortable']) ||
					$this->custom_columns[$column['field']]['sortable'])) {
				unset($column['field']);
			}
		}

		return $header;
	}

	private function formatheader($header) {
		if (is_callable($this->option_callback)) {
			$header[] = array('data' => t('Option'), 'field' => null, 'class' => array('no-print'));
		}

		if ($this->serial_col) {
			array_splice($header, 0, 0, array(array('data' => t('S/N'), 'field' => 'S/N')));
		}
		return $header;
	}
	
	/**
	 * Takes the header for a table, and returns the SQL alias containing that
	 * column, in case there are column name conflicts after a join.
	 * 
	 * @param array $header
	 * @param SelectQueryInterface $sql
	 */
	private function mapHeaderToQueryAlias($header, $sql) {
		//Find all aliases in the query, because of column name conflicts.
		$alias_map = array();
		foreach ($sql->getFields() as $alias => $column) {
			$alias = isset($column['alias']) ? $column['alias'] : $column['field'];
			if (empty($column['table'])) {
				$alias_map[$column[field]] = $alias;
			} else {
				$alias_map[sprintf('%s.%s', $column['table'], $column['field'])] = $alias;
			}
		}
		
		$lookup = empty($header['table']) ? $header['field'] :
			sprintf('%s.%s', $header['table'], $header['field']);
		return isset($alias_map[$lookup]) ? $alias_map[$lookup] : $header['field'];
	}

	public function getBasicSearchQuery(&$form_state) {
		$values = $this->getValues($form_state);
		return $this->getBasicSearchQueryInternal($values);
	}

	private function getBasicSearchQueryInternal(&$values) {
		$sql = $this->basic_search_query; //this is now an unexecuted query object

		if (isset($values['search_field']))
			$str = $values['search_field'];
		else
			$str = '';

		$param = array();
		if(isset($values['condition_field'])){
			$condition = $values['condition_field'];
			$param[':d'] = $condition;
		}

		// TODO : Change this to accept arbitrary number of placeholders
		if (!$this->disable_basic_search && !empty($this->basic_search_where_clause)) {
			$param[':str'] = '%' . $str .'%';
			if (strpos($this->basic_search_where_clause, ':str2') !== false) {
				$param[':str2'] = '%' . $str . '%';
			}

			$sql->where($this->basic_search_where_clause,$param);
		}
		return $sql;
	}
	
	private function getBasicSearchOutput($values) {
		$headers = $this->basic_search_header;
		$this->table_header = $headers;
		$this->table_data = array();
		$this->export_header = $headers;
		
		$pagelimit = $this->basic_search_pagelimit;

		$sql = clone $this->getBasicSearchQueryInternal($values);
	    $this->export_query = serialize($sql);
        //count number of results
        $count = $sql->countQuery()
	        		 ->execute()
	        		 ->fetchField();

		$sql=$sql->extend('PagerDefault')
			->limit($pagelimit);
		$sql=$sql->extend('TableSort')
			->orderByHeader($this->removedynamic($headers));

		$result = $sql->execute();
		$headers=$this->formatheader($headers);

		$row = array();
		$rows = array();
		$page = pager_find_page();
		$entrycount = $this->basic_search_pagelimit * $page + 1;
		while ($data = $result->fetchAssoc())
		{
			foreach($headers as $key=>$val){
				$column_name = $this->mapHeaderToQueryAlias($val, $sql);
				
				if(isset( $this->custom_columns[$column_name] )){
					$field = $this->custom_columns[$column_name];
					$row[$column_name] = is_callable($field) ?
						call_user_func($field, $data) : call_user_func($field['display'], $data);
				} else if ($val['data'] == 'S/N') {
					$row[$column_name] = $entrycount;
				} else if($val['data'] == 'Option'){
					$row[$column_name] = array(
						'class' => array('no-print'),
						'data' => call_user_func($this->option_callback, $data)
					);
				} else if (!empty($data[$column_name]) && array_key_exists($column_name, $this->codem_options)){
					$row[$column_name] = $this->codem_options[$column_name][$data[$column_name]];
				} else {
					$row[$column_name] = $data[$column_name];
				}
			}
			$this->table_data[]  = $row;
			$rows[] = array('data' =>$row);
			$entrycount++; }

		$output = array(array('#markup' => 'Total Number of Records: <b>' . $count . '</b>&nbsp;  '));
		$this->form['excel_export'] = array(
			'#type'=>'submit',
			'#name'=>'excel_export',
			'#value'=>t('Export to Excel'),
			'#attributes' => array(
				'class' => array(
					'no-print'
				)
			)
		);
		$output[] = &$this->form['excel_export'];
		$output[] = array('#theme' => 'table', '#header' => $this->removedynamic($headers),
						 '#rows' => $rows,
						 '#empty' => 'No entries found');
		$output[] = array('#theme' => 'pager');

		return $output;
	}

	public function getAdvancedSearchQuery(&$form_state) {
		$values = $this->getValues($form_state);
		$headers = null;
		return $this->getAdvancedSearchQueryInternal($values, $headers);
	}

	private function getAdvancedSearchQueryInternal(&$values, &$headers) {
		$headers  = $this->adv_search_default_header;
		//Add options as last header
		//initialize query string
		$count = 0;
		$firstjoin;
		$firsttable;
		$query = $this->initAdvancedSearchQuery($firstjoin, $firsttable);
		foreach($this->select_array as $table => $columns) {
			//Ignore dynamically generated columns.
			if(empty($table)) {
				continue;
			}

			$join = $this->selectJoin($table);
			//$select .=$table . "." . $column.", ";
			if(empty($query)){
				$firsttable = $table;
				$query = \CVWO\Base\Database\select($table);
				$this->checkTable($table);
				$firstjoin = $this->selectJoin($table);
				if ($firstjoin) {
					$tableColumn = $this->checkTableColumn($join,$table);
					if ($tableColumn){
						$query->fields($table,array($tableColumn));
					}
				}
			}
			$query->fields($table, array_keys($columns));
			if($this->checkTable($table) && $join && $firstjoin){
				$query->join($table,$table,$firsttable.'.'.$firstjoin .'='. $table .'.'.$join);
			}
			$count +=1;
		}

		//custom fields
		foreach($this->custom_columns as $table => $fields) {
			if (!is_array($fields)) {
				//Old format. Who cares.
				continue;
			}
			
			if (isset($fields['query']) && is_callable($fields['query'])) {
				$query = call_user_func($fields['query'], $query);
				assert(!empty($query) && ($query instanceof SelectQuery));
			}
		}

		//display fields
		foreach($this->display_fields as $table => $fields) {
			foreach($fields as $field=> $title) {
				if(isset($values[$field.'-display']) && $values[$field.'-display'] == 1 ){
					$header = array('data'=>$title, 'field'=>$field);
					if(!array_key_exists($field, $this->select_skip_array)){
						$addfield = $this->checkTableColumn($field, $table);
						$addtable = $this->checkTable($table);
						$addjoin = $this->selectJoin($table);
						if($addfield){
							$header['field'] = $query->addField($table, $addfield);
						}
						if($addtable && $addjoin){
							$query->join($addtable,$addtable,$firsttable.'.'.$firstjoin.'='.$addtable.'.'.$addjoin);
						}
					}
					$headers[] = $header;
				}
			}
		}
		$this->export_header = $headers;
		foreach($this->hidden_select_fields as $fields => $column) {
			$addfield = $this->checkTableColumn($column['field'], $column['table']);
			$addtable = $this->checkTable($column['table']);
			$addjoin = $this->selectJoin($addtable);
			if($addfield){
				$query->fields($column['table'],array($addfield));
			}
			if($addtable && $firstjoin && $addjoin){
				$query->join($addtable,$addtable,$firsttable.'.'.$this->selectJoin($firsttable).'='.$addtable.'.'.$this->selectJoin($addtable));
			}
		}
		$this->table_header = $headers;

		//custom filter
		foreach($this->custom_filters as $id => $filter) {
			if(!empty($values['customFilter_'.$id])) {
				$query = call_user_func_array($filter['#query_callback'], array(&$query, $values['customFilter_'.$id]));
				assert(!empty($query) && ($query instanceof SelectQueryInterface));
				if(isset($filter['#column']) && !array_key_exists($filter['#column'], $this->select_skip_array)
				&& strlen($filter['#column']) > 0){
					$addfield = $this->checkTableColumn($filter['#column'], $filter['#table']);
					$addtable = $this->checkTable($filter['#table']);
					$addjoin = $this->selectJoin($addtable);
					if($addfield){
						$query->fields($filter['#table'],array($addfield));
					}
					if($addtable && $firstjoin && $addjoin){
						$query->join($addtable,$addtable,$firsttable.'.'.$firstjoin.'='.$addtable.'.'.$addjoin);
					}
				}
			}
		}
		//normal filter
		foreach($this->filter_list as $table => $filters) {
			for($i=0; $i < count($filters); $i++) {
				$filter = $filters[$i][0];
				if(!empty($values[$filter])) {
					$query->condition($table . "." . $filter,$values[$filter]);
					$addtable = $this->checkTable($table);
					$addjoin = $this->selectJoin($addtable);
					if($addtable && $firstjoin && $addjoin){
						$query->join($addtable,$addtable,$firsttable.'.'.$firstjoin.'='.$addtable.'.'.$addjoin);
					}
				}
			}
		}

		//append default where clause
		if (!empty($this->default_where_clause)) {
			$query->where($this->default_where_clause);
		}

		//Join tables together
		$tables = $this->selected_tables;

		if($this->group_by!='')
			$query->groupBy($this->group_by);
		$this->export_query = serialize($query);
		return $query;
	}

	private function initAdvancedSearchQuery(&$firstjoin, &$firsttable) {
		if (isset($this->advancedSearchQuery) && !empty($this->advancedSearchQuery)) {
			$query = $this->advancedSearchQuery;	
			$fields = $query->getFields();
			$tables = $query->getTables();

			foreach ($tables as $t => $v) {
				$this->checkTable($t);

				if (empty($firsttable)) {
					$firsttable = $t;
					$firstjoin = $this->selectJoin($firsttable);
				}
			}

			foreach ($fields as $f => $v) {
				$this->checkTableColumn($v['field'],$v['table']);
			}
			return $query;	
		} else {
			$firsttable = FALSE;
			$firstjoin = FALSE;
			return '';
		}
	}

	private function getAdvancedSearchOutput(&$values) {
		$headers = null;
		$query = clone $this->getAdvancedSearchQueryInternal($values, $headers);
		$count = $query->countQuery()->execute()->fetchField();

		$pagelimit = ($values['pagelimit'] > 0 ? $values['pagelimit'] : $this->basic_search_pagelimit);

		$page = pager_default_initialize($count, $pagelimit);
		$query=$query->extend('TableSort')
					// ->orderByHeader($headers)
					->orderByHeader($this->removedynamic($headers))
				  	->extend('PagerDefault')
					->limit($pagelimit);

		$result = $query->execute();

		$headers = $this->formatheader($headers);

		$entrycount = $pagelimit * $page + 1;
		$rows = array();
		while ($data = $result->fetchAssoc()){
			foreach($headers as $key=>$val){
				$column_name = $this->mapHeaderToQueryAlias($val, $query);
					
				if(isset( $this->custom_columns[$column_name] )){
					$field = $this->custom_columns[$column_name];
					$row[$column_name] = is_callable($field) ?
						call_user_func($field, $data) : call_user_func($field['display'], $data);
				} else if ($val['data'] == 'S/N') {
					$row[$column_name] = $entrycount;
				} else if($val['data'] == 'Option'){
					$row[$column_name] = array(
						'class' => array('no-print'),
						'data' => call_user_func($this->option_callback, $data)
					);
				} else if (!empty($data[$column_name]) && array_key_exists($column_name, $this->codem_options)){
					$row[$column_name] = $this->codem_options[$column_name][$data[$column_name]];
				} else {
					$row[$column_name] = $data[$column_name];
				}
			}
			$this->table_data[] = $row;
			$rows[] = array('data' => $row);
			$entrycount++;
		}
		$output = array(array('#markup' => "Total Number of Records: <b>" . $count . '</b>&nbsp;  '));
		$this->form['excel_export'] = array(
			'#type'=>'submit',
			'#name'=>'excel_export',
			'#value'=>t('Export to Excel'),
			'#attributes' => array(
				'class' => array(
					'no-print'
				)
			)
		);
		$output[] = &$this->form['excel_export'];
		$output[] = array('#theme' => 'table', '#header' => $this->removedynamic($headers),
						 '#rows' => $rows,
						 '#empty' => 'No entries found');
		$output[] = array('#theme' => 'pager');
		return $output;
	}

	public function exportToExcel() {
		$sql = unserialize($this->export_query);
		$result = $sql->execute();
		$headers = $this->export_header;
		
		$entrycount = 1;
		if ($this->serial_col) $headers = (array_merge(
			array(array('data' => t('S/N'), 'field' => t('S/N'))),
			$headers));
		$rows = array();
		while ($data = $result->fetchAssoc()){
			foreach($headers as $key=>$val){
				$column_name = $this->mapHeaderToQueryAlias($val, $sql);
				
				if(isset( $this->custom_columns[$column_name] )){
					$field = $this->custom_columns[$column_name];
					$row[$column_name] = is_callable($field) ?
						call_user_func($field, $data) : call_user_func($field['display'], $data);
				} else if ($val['data'] == 'S/N') {
					$row[$column_name] = $entrycount;
				} else if($val['data'] == 'Option'){
					$row[$column_name] = array(
						'class' => array('no-print'),
						'data' => call_user_func($this->option_callback, $data)
					);
				} else if (!empty($data[$column_name]) && array_key_exists($column_name, $this->codem_options)){
					$row[$column_name] = $this->codem_options[$column_name][$data[$column_name]];
				} else {
					$row[$column_name] = $data[$column_name];
				}
			}
			$this->table_data[] = $row;
			$rows[] = array('data' => $row);
			$entrycount++;
		}
		$filename = $this->search_name . ' ' . date("Y-m-d");
		cvwobase_download_as_excel($headers, $rows, $filename);
	}
	
	/*
	* check whether the table has already been selected
	*/
	private function checkTable($table) {
		if (!in_array($table, $this->selected_tables)){
			$this->selected_tables[] = $table;
			return $table;
		}
		return FALSE;
	}

	/*
	 * check whether a column of a table has been selected. Adds column to
	 * selected if not selected.
	 * 
	 * @param string $column The column to look up
	 * @param string $table The table the column belongs to. Null for expressions.
	*/
	private function checkTableColumn($column, $table) {
		//First, check if we have seen the table name before
		if (!array_key_exists($table, $this->select_array)) {
			$this->select_array[$table][$column] = true;
			return $column;
		} else if (!array_key_exists($table, $this->select_array[$table])) {
			//We have seen the table before. See if we've seen the column before.
			$this->select_array[$table][$column] = true;
			return $column;
		} else {
			//Seen before. No fun.
			return false;
		}
	}

	/*
	* Selects the join column for the table. Returns the join column if exists, else false
	*/
	private function selectJoin($table) {
		if (empty($table)) {
			return FALSE;
		}

		if (is_string($this->join_column) && $this->join_column != '') {
			return $this->join_column;
		} else if (is_array($this->join_column)) {
			if (array_key_exists($table, $this->join_column)) {
				return $this->join_column[$table];
			}
		} else {
			return FALSE;
		}
	}
}