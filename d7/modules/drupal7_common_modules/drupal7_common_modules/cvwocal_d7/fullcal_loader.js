/**
 * @file
 * Loads Full Calendar using specified settings
 *
 * Usage: Attach the array fullcalendar_settings to the settings using $form['#attached']['js']
 *  The following fields should be specified:
 *  - id: ID of the calendar container.
 *  - event_source: URL for retrieving events
 *  - edit_url: URL for editing events by dragging and resizing
 *  - drop_tag: Tag used by edit_url to signify dragging and dropping (moving the event)
 *  - resize_tag: Tag used by edit_url to signify resizing
 *  - error_tag: Key of a value returned by edit_url in JSON format. If the value is TRUE, there was an update error
 *  - event_type_selector: ID of the select field used to specify the type of a new event
 *  - empty_event: Value of the 'Please Select' option in event_type_selector
 */
(function ($) {
  Drupal.behaviors.fullCalendarLoad = {
    attach: function(context, settings) {
      var cal_settings = settings.fullcalendar_settings;
      var id = cal_settings.id;
      var empty_event = cal_settings.empty_event;
      var type_selector = cal_settings.event_type_selector;
      var error_tag = cal_settings.error_tag;
      var upd_err_msg = Drupal.t('There was an error while trying to update the event. Please edit the event manually by clicking on its name.');
      var error_func = function(event, revertFunc) {
        revertFunc();
        alert(upd_err_msg);
      }
      var update_func = function(url, event, revertFunc) {
        event.element.qtip('disable');
        event.element.qtip('hide');
        $.ajax({
          url: url,
          cache: false,
          dataType: 'json',
          error: function() {
            error_func(event, revertFunc);
          },
          success: function(data) {
            if (data[error_tag] === true)
              error_func(event, revertFunc);
          },
          complete: function() {
            event.element.qtip('hide');
            event.element.qtip('enable');
          }
        });
      }
	  var start = '';
	  var end = '';
	  $('#event_form').dialog({
		autoOpen: false,
		draggable: false,
		modal: true,
		buttons: [{
			text: Drupal.t('Ok'),
			click: function() {
				var chosen_type_url = $(type_selector).val();
				if (chosen_type_url == empty_event) {
					alert(Drupal.t('Please choose an event'));
				} else {
					$(this).dialog('close');
					window.location = settings.basePath + chosen_type_url + '/' + '/' + start + '/' + end;
				}
			},
		}],
	  });
      var fc_settings = {
        header: {
          left: 'prev,next today',
          center: 'title',
          right: 'month,agendaWeek,agendaDay'
        },
        firstHour: 8,
        editable: true,
        selectable: true,
        selectHelper: true,
        unselectAuto: false,
        allDayDefault: false,
        eventDrop: function(event, dayDelta, minuteDelta, allDay, revertFunc) {
          update_func(cal_settings.edit_url + '/' + event.type + '/' + event.module + '/' + event.internal_id + '/' + cal_settings.drop_tag + '/' + dayDelta + '/' + minuteDelta + '/' + (allDay ? 1 : 0), event, revertFunc);
        },
        eventResize: function(event, dayDelta, minuteDelta, revertFunc) {
          update_func(cal_settings.edit_url + '/' + event.type + '/' + event.module + '/' + event.internal_id + '/' + cal_settings.resize_tag + '/' + dayDelta + '/' + minuteDelta, event, revertFunc);
        },
        eventRender: function(event, element) {
          event.element = element;
          event.qtipoptions = {
            content: event.qtip_content,
            tip: true,
            solo: true,
            show: {
              delay: 200
            },
            hide:{
              delay: 100,
              fixed: true
            },
            position: {
              corner :{
              target:'leftTop',
              tooltip: 'rightMiddle'
            },
              adjust:{screen: true},
            }
          }

          element.qtip(event.qtipoptions);
        },
        eventDragStart: function(event) {
          event.element.qtip('disable');
          event.element.qtip('hide');
        },
        eventDragStop: function(event) {
          event.element.qtip('hide');
          event.element.qtip('enable');
        },
        eventResizeStart: function(event) {
          event.element.qtip('disable');
          event.element.qtip('hide');
        },
        eventResizeStop: function(event) {
          event.element.qtip('hide');
          event.element.qtip('enable');
        },
		select: function(startDate, endDate, allDay, jsEvent, view) {
			start = startDate.getTime()/1000; // passes back in seconds, instead of millseconds
			end = endDate.getTime()/1000; // passes back in seconds, instead of milliseconds
			$('#event_form').dialog('open');
		},
		eventClick: function(event) {
			window.location = settings.basePath + event.edit_url + '/' + event.internal_id + '/' + '/';
		},
      };
      fc_settings.eventSources = [{url: cal_settings.event_source}];
      if (cal_settings.year != undefined) {
        fc_settings.year = cal_settings.year;
        fc_settings.month = cal_settings.month;
        fc_settings.day = cal_settings.day;
      }
      $('#' + id, context).fullCalendar(fc_settings);
    },
    detach: function(context, settings) {
      $('#' + settings.fullcalendar_settings.id, context).fullCalendar('destroy');
    }
  };
}) (jQuery);