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
	var inLightbox = false;

	CVWO.Community = {
		AAS: {
			selectPerson: function(json) {
				var $ = window.parent.jQuery;
				var person = $('input#person_id');

				person.val(json);
				person.trigger('autocompleteSelect');

				$.fancybox.close();
			}
		}
	};

	$(document).ready(function() {
		//We will attach the same JS to both the main form as well as the Search form.
		inLightbox = window.parent !== window &&
			window.parent.jQuery('.search_by_address');

		if (!inLightbox) {
			$('.search_by_address').click(function() {
				$.fancybox(Drupal.settings.basePath + '/cvwo_community/aas/address_search',
					{type: 'iframe'});
				return false;
			});
		}
	});
})(jQuery);
