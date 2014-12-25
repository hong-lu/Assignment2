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
		var personInfo = JSON.parse(this.raw_value);

		//Build the address to display. The server will give us ALL addresses,
		//but just pick the first one.
		if (personInfo.address.length !== 0) {
			var addressInfo = personInfo.address[0];

			$('#address_display')[0].value = addressInfo.display_text;
			$('#address_id')[0].value = addressInfo.address_id;
		} else {
			$('#address_display')[0].value = '';
			$('#address_id')[0].value = ''
		}

		//Set the person's contact information.
		$('#contact_display > tbody *').remove();
		var contactDisplay = $('#contact_display > tbody');
		for (var i = 0; i < personInfo.contact.length; ++i) {
			var tableRow = $('<tr><td>' + personInfo.contact[i].contact_type_display +
				'</td><td>' + personInfo.contact[i].contact_value +
				'</td></tr>');
			if (i % 2 === 0) {
				tableRow.attr('class', 'odd');
			} else {
				tableRow.attr('class', 'even');
			}
			contactDisplay.append(tableRow);
		}
	}

	//Hook our autocomplete listener.
	$(document).ready(function() {
		$('input#person_id').bind('autocompleteSelect', onPersonAutocomplete);
	});
})(jQuery);
