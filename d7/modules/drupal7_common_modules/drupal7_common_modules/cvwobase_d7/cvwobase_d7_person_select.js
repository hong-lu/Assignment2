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

(function($) {
	//Define our autocomplete listener
	//"this" will be the element which received the autocomplete.
	function onPersonAutocomplete() {
		var personInfo = JSON.parse(this.value);
		this.raw_value = this.value;
		this.value = personInfo.name;

		//Set the person ID
		$('input#' + this.id + '_id')[0].value = personInfo.person_id;
	}

	function onPersonChange(e) {
		if (e.keyCode === 13 /*\n*/) {
			return true;
		}

		$('input#' + this.id + '_id')[0].value = '';
	}

	//Hook our autocomplete listener.
	Drupal.behaviors.CVWO_Select_Person = {
		attach: function(context) {
			$('input.form-select-person', context).bind('autocompleteSelect', onPersonAutocomplete);
			$('input.form-select-person', context).bind('keyup', onPersonChange);
		}
	};
})(jQuery);
