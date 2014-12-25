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

(function() {
	CVWO.Base.form_multiple_input = {
		/**
		 * Constructor for a new field.
		 *
		 * @param string sentinel The ID of the node which will have the new
		 *                        elements prepended to.
		 * @param string html The HTML to append when a new entry is requested.
		 * @param number count The number of elements the field currently has.
		 */
		field: function(sentinel, html, count) {
			this._sentinel = sentinel;
			this._template = html;
			this._count = count;
		},

		/**
		 * The fields which are multiple-input.
		 */
		fields: {},

		/**
		 * Adds a new input line for the given field.
		 *
		 * The generated field will have a div wrapping the entry. The div will
		 * have an ID which is the sentinel of the form, appended with 2 hyphens
		 * and the index in the list.
		 * 
		 * @param string fieldid The ID of the field to add. This is the key to
		 *                       the elements in @see fields.
		 */
		add: function(fieldid) {
			var augmentNameWithIndex = function(element, index) {
				if (!element.name) {
					return;
				}

				var bracketIndex = element.name.indexOf('[');
				if (bracketIndex === -1) {
					element.name += index;
				} else {
					element.name = element.name.substr(0, bracketIndex) + index +
						element.name.substr(bracketIndex);
				}
			};

			//Get the parent that we are to insert the template code into.
			var self = CVWO.Base.form_multiple_input;
			var field = self.fields[fieldid];

			//Set the IDs of the new elements
			++field._count;
			var node = jQuery.parseHTML(field._template)[0];
			var elements = jQuery('select, textarea, ' +
				'input:not(".autocomplete, .form-select-person-id, ' +
				'.form-select-organisation-id, ' +
				'.form_multiple_input_indexer, .form-type-date-popup input"), ' +
				'.form_multiple_input_entry', node);
			//Also set the ID of the root node. We will be deleting it.
			elements.push(node);
			elements.each(function(index, elem) {
				elem.id += field._count;
				augmentNameWithIndex(elem, field._count);
			})

			//Indexers need the ID in their VALUE.
			var indexers = jQuery('.form_multiple_input_indexer', node);
			indexers.each(function(index, elem) {
				elem.id += field._count;
				elem.value = field._count;
			});

			//If we have autocomplete fields, Modify the IDs specially.
			var autocompletes = jQuery('.form-autocomplete', node);
			jQuery('.autocomplete', node).each(function(index, elem) {
				elem.id = elem.id.substr(0, elem.id.length - 13) + field._count + '-autocomplete';
				augmentNameWithIndex(elem, field._count);
			});
			var select_person_ids = jQuery('.form-select-person-id', node);
			select_person_ids.each(function(index, elem) {
				elem.id = elem.id.substr(0, elem.id.length - 3) + field._count + '_id';
				augmentNameWithIndex(elem, field._count);
			});
			var select_organisation_ids = jQuery('.form-select-organisation-id', node);
			select_organisation_ids.each(function(index, elem) {
				elem.id = elem.id.substr(0, elem.id.length - 3) + field._count + '_id';
				augmentNameWithIndex(elem, field._count);
			});

			//If we have date_popups, then enable interactivity.
			var date_popups = jQuery('.form-type-date-popup', node);
			date_popups.each(function(index, elem) {
				//Get the base class for this popup.
				var itemClass = null;
				for (var i = 0; i < elem.classList.length; ++i) {
					if (elem.classList[i].substr(0, 10) === 'form-item-') {
						itemClass = elem.classList[i];
						break;
					}
				}

				//Find the date element
				jQuery('.' + itemClass + '-date input', elem).each(function(index, elem) {
					var templateName = elem.id;
					elem.id += field._count;
					augmentNameWithIndex(elem, field._count);

					Drupal.settings.datePopup[elem.id] =
						Drupal.settings.datePopup[templateName]
				});

				//Find the time element
				jQuery('.' + itemClass + '-time input', elem).each(function(index, elem) {
					var templateName = elem.id;
					elem.id += field._count;
					augmentNameWithIndex(elem, field._count);

					Drupal.settings.datePopup[elem.id] =
						Drupal.settings.datePopup[templateName]
				});
			});

			//Insert the template code.
			jQuery('#' + field._sentinel).before(node);

			//Hook Drupal's behaviours to the newly created node.
			Drupal.attachBehaviors(node);
		},

		/**
		 * Removes the id'th element from the given fieldid.
		 * 
		 * @param string fieldid The ID of the field to remove the input from.
		 * @param number id The id of the element to remove. This is generated
		 *                  by the client-side code based on the number of
		 *                  elements in the field at the time the element was
		 *                  created. Therefore, the ID may not be in running
		 *                  order (if elements have been deleted)
		 */
		remove: function(fieldid, id) {
			//Deduce the ID of the field to remove.
			id = fieldid + '__mi__' + id;

			//Reduce the number of
			jQuery('#' + id).remove();
		},

		/**
		 * Removes the element with the given HTML ID. The HTML ID follows a
		 * special format so we can deduce the field ID and index from the ID.
		 *
		 * @param string id The HTML ID of the element to remove. This is the
		 *                  ID of the "remove" link.
		 */
		remove_using_id: function(id) {
			var self = CVWO.Base.form_multiple_input;
			var index = id.indexOf('__mi_r__');

			return self.remove(id.substr(0, index),
				parseInt(id.substr(index + 8)));
		}
	};
})();
