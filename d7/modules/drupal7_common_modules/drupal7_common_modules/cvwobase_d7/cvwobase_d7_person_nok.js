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
	function onAutocompleteSelect() {
		var personInfo = JSON.parse(this.value);
		this.value = this.id === 'edit-nric-fin' ? personInfo.nric_fin :
			personInfo.name;

		//We need to jump to the person. First, get our URI. Slice off anything
		//after # or ?
		var uri = location.href;
		var uriAnchor = uri.indexOf('#');
		if (uriAnchor !== -1) {
			uri = uri.substr(0, uriAnchor);
		}
		var uriQuery = uri.indexOf('?');
		if (uriQuery !== -1) {
			uri = uri.substr(0, uriQuery);
		}

		//Find our path components. Normalise, removing any trailing /.
		var path = uri.split('/');
		var trailingSlash = path[path.length - 1] === '';
		if (trailingSlash) {
			path.splice(-1, 1);
		}

		//If we ended with add, we need to go one level down.
		$.fancybox.showLoading();
		if (path[path.length - 1] === 'add') {
			//Go one level deeper.
			location.href = (trailingSlash ? '' : 'add/') +
				personInfo.person_id + '/';
		} else if (path[path.length - 2] === 'add') {
			//Same level.
			location.href = (trailingSlash ? '../' : './') +
				personInfo.person_id;
		}
	}
	
	$(document).ready(function() {
		$('div.form-item-nric-fin > input, div.form-item-name > input').bind(
			'autocompleteSelect', onAutocompleteSelect);
	});
})(jQuery);
