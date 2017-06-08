<?php

/**
 * @file
 * Page callbacks and relevant forms (and friends).
 */
 

function islandora_qa_task_page_callback($taskId) {
  module_load_include('inc', 'islandora_qa', 'includes/db');
  
  $nextId = islandora_qa_get_next_in_task($taskId);
  if($nextId == null)
    return drupal_get_form('islandora_qa_task_complete_form');
  
  $object = islandora_object_load($nextId);
  
  $image = islandora_datastream_load(islandora_qa_get_stream_look($taskId), $object);
  
  $dataStream = islandora_qa_get_stream_edit($taskId);
  
  $formId = islandora_qa_get_form($taskId);

  return drupal_get_form('islandora_qa_datastream_form', $taskId, $object, $dataStream, $formId);
}

function islandora_qa_task_complete_form(array $form, array &$form_state) {
  $form = array();
  $form['Hardcode'] = array(
    '#markup' => 'Task is done. This page should have other stuffs'
  );
  return $form;
}

function islandora_qa_datastream_form(array $form, array &$form_state, $taskId, AbstractObject $object, $dsid, $formId) {
  form_load_include($form_state, 'inc', 'xml_form_builder', 'includes/datastream.form');
  
  // Leave this here for legacy reasons.
  $form_state['datastream'] = isset($object[$dsid]) ? $object[$dsid] : FALSE;
  
  $associations = xml_form_builder_datastream_form_get_associations($form_state, $object->models, $dsid);
  //Code to get an association
  //start with a fallback
  $association = xml_form_builder_datastream_form_get_association($form_state, $associations);
  //get an association
  foreach ($associations as $a) {
        if (array_key_exists('id', $a) && $a['id']==$formId) {
            $association = $a;
        }
    }
  

  return isset($association) ?
    islandora_qa_task_item_form($form, $form_state, $taskId, $object, $association) :
    xml_form_builder_datastream_form_select_association_form($form, $form_state, $associations);
}

function islandora_qa_task_item_form(array $form, array &$form_state, $taskId, AbstractObject $object, array $association) {
  $form_state['pid'] = $object->id;
  $form_state['association'] = $association;
  $form_name = $association['form_name'];
  $dsid = $association['dsid'];
  $xml = NULL;
  if (isset($object[$dsid])) {
    $xml = $object[$dsid]->content;
  }
  
  $progress = islandora_qa_get_progress_in_task($taskId);
  
  $subForm = xml_form_builder_get_form($form, $form_state, $form_name, $xml);
  $form = array();
  
  $form['progress'] = array(
    '#markup' => 'Number '.($progress[0]+1).' of '.$progress[1]
  );
  
  //TODO Look into other ways to display images
  $form['image'] = array(
    '#markup' => theme('image', array('path' => 'islandora/object/' .($object -> id). '/datastream/' .(islandora_qa_get_stream_look($taskId)),
      'attributes' => array('class' => 'side-image'))),
  );
  
  $form['Hardcode'] = array(
    '#markup' => '<div id="" style="overflow-y: scroll; height:400px;">'
  );
  $form = array_merge($form, $subForm);
  $form['done_hardcoding'] = array(
    '#markup' => '</div>'
  );
  
  // Make the Object label field required.
  if ($association['title_field']) {
    $title_field_parents = $association['title_field'];
    $title_field_parents[] = '#required';
    drupal_array_set_nested_value($form, $title_field_parents, TRUE);
  }
  
  $form['taskId'] = array(
		'#type' => 'hidden',
		'#value' => ($taskId),
	);
	
	$form['itemId'] = array(
		'#type' => 'hidden',
		'#value' => ($object -> id),
	);
  
  $form['update'] = array(
    '#type' => 'submit',
    '#value' => t('Update'),
    '#name' => 'update',
    '#submit' => array('islandora_qa_task_item_form_submit'),
  );
  
  $form['reject'] = array(
    '#type' => 'link',
    '#href' => ('islandora_qa/task/'.$taskId.'/reject/'.($object -> id)),
    '#title' => t('reject'),
  );
  
  return $form;
}
/**
 * Updates the datastream's content with generated xml from the metadata form.
 *
 * Also updates the objects label, and applies a transform to update the DC.
 *
 * @param array $form
 *   The drupal form.
 * @param array $form_state
 *   The drupal form state.
 */
function islandora_qa_task_item_form_submit(array $form, array &$form_state) {
  $object = islandora_object_load($form_state['pid']);
  $association = $form_state['association'];
  $label = NULL;
  if ($association['title_field']) {
    $title_field = drupal_array_get_nested_value($form, $association['title_field']);
    $label = $title_field['#value'];
  }
  $xml_form = new XMLForm($form_state);
  $document = $xml_form->submit($form, $form_state);
  xml_form_builder_update_object($object, $association, $document, $label);
  
  islandora_qa_mark_done_in_task($form_state['values']['taskId'], $form_state['values']['itemId']);
}

function islandora_qa_task_item_form_reject(array $form, array &$form_state) {
  drupal_goto('islandora_qa/task/'.($form_state['values']['taskId']).'/reject/'.($form_state['values']['itemId']));
}


/**
 *
 *
 *
 *
 *
 */
function islandora_qa_task_reject_page_callback ($taskId, $itemId) {
  module_load_include('inc', 'islandora_qa', 'includes/db');

  return drupal_get_form('islandora_qa_reject_form', $taskId, $itemId);
}

function islandora_qa_reject_form(array $form, array &$form_state, $taskId, $itemId) {
  $form = array();

  $form['taskId'] = array(
		'#type' => 'hidden',
		'#value' => ($taskId),
	);
  
  $form['itemId'] = array(
    '#type' => 'hidden',
    '#default_value' => ($itemId),
  );
  
  $form['reason'] = array(
    '#type' => 'textfield',
    '#title' => t('Reason'),
    '#size' => 100,
  );
  
  $form['reject'] = array(
    '#type' => 'submit',
    '#value' => t('Reject'),
    '#name' => 'reject',
    '#submit' => array('islandora_qa_reject_form_submit'),
  );
  
  $form['cancel'] = array(
    '#type' => 'link',
    '#href' => ('islandora_qa/task/'.$taskId),
    '#title' => t('cancel'),
  );
  
  return $form;
}

function islandora_qa_reject_form_submit(array $form, array &$form_state) {
  
  islandora_qa_reject_in_task($form_state['values']['taskId'], $form_state['values']['itemId'], $form_state['values']['reason']);
  
  drupal_goto('islandora_qa/task/'.($form_state['values']['taskId']));
}