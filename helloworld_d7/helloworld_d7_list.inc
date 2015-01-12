<?php 

namespace CVWO\HelloWorld {
	require_once drupal_get_path("module", MODULE_NAME)."/helloworld_d7_api.inc";

	/**
	 * Displays the form for searching the user records
	 */
	function helloworld_list_form($form, &$form_state, $name = ''){

		$form['name'] = array(
			'#type' => 'textfield',
			'#title' => t('Search Person'),
			'#size' => 32,
			'#default_value' => $name,
			'#autocomplete_path' => MODULE_NAME . '/name_autocomplete',
		);
		
		$form['search_submit'] = array(
			'#type' => 'submit',
			'#value' => t('Search'),
		);

		$form['output'] = array(
			'#markup' => search_output($name),
		);
		return $form;
	}

	/**
	 * Submit handler for helloworld_list_form
	 * Redirects the page along with parameters, when submit button is pressed
	 */
	function helloworld_list_form_submit($form, &$form_state){
		 $form_state['redirect'] = MODULE_NAME . '/list/' . $form_state['values']['name'];
	}

	/**
	 * Themeable function for helloworld_list_form
	 * Returns html for the form.
	 * 
	 * @param $form
	 *
	 * @ingroup themeable
	 */
	function theme_helloworld_list_form($variables) {
		$form = $variables['form'];

		$output = '<table width=400 border=0 rules="none" cellspacing=0 cellpadding=0>
				<tr><td width=300>' . drupal_render($form['name']) . '</td>
					<td style="padding-top:20px">' . drupal_render($form['search_submit']) . '</td>
				</tr>
			</table>';
		
		// Render the rest of the form, as there may be hidden elements that Drupal needs to render
		// for its own use. Note that it doesn't re-render the elements that you have already rendered
		$output .= drupal_render_children($form);
		return $output;
	}
}

