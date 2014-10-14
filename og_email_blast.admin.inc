<?php
/**
 * @file
 * Admin settings for Organic groups e-mail blast module.
 */

/**
 * E-mail group members form.
 */
function og_email_blast_group($form, &$form_state, $entity_type, $etid) {
  global $user;
  og_set_breadcrumb($entity_type, $etid, array(l(t('Group'), "$entity_type/$etid/group")));
  if (og_is_group($entity_type, $etid)) {
    $entities = entity_load($entity_type, array($etid));
    $entity = reset($entities);
    $form['intro'] = array(
      '#markup' => '<p>' . t('Use this form to send an e-mail message to group members.') . '</p>',
    );
    $form['entity_type'] = array(
      '#type' => 'value',
      '#value' => $entity_type,
    );
    $form['etid'] = array(
      '#type' => 'value',
      '#value' => $etid,
    );
    list(,,$bundle_name) = entity_extract_ids($entity_type, $entity);
    $roles = og_roles($entity_type, $bundle_name, $etid);
    $role_options = array();
    $perm_send_to_all = og_user_access($entity_type, $etid, 'email people');
    $perm_send_to_admins = og_user_access($entity_type, $etid, 'email administrators');
    foreach ($roles as $rid => $role_name) {
      if ($role_name == OG_ANONYMOUS_ROLE) {
        continue;
      }
      if ($role_name == OG_AUTHENTICATED_ROLE && $perm_send_to_all) {
        $role_options['all'] = t('All the members');
      }
      elseif ($perm_send_to_all || ($perm_send_to_admins && $role_name == OG_ADMINISTRATOR_ROLE)) {
        $role_options[$rid] = t($role_name, array(), array('context' => 'og_blast_email_role_option'));
      }
    }
    // If there is only one option automatically select it
    // e.g. when we give anonymous the permission to send message only to admins
    $default_recipients = null;
    if (count($role_options) == 1) {
      $role_id = array_keys($role_options);
      $default_recipients = $role_id[0];
    }
    $form['roles'] = array(
      '#type' => 'radios',
      '#options' => $role_options,
      '#default_value' => 'all',
      '#title' => t('Recipients'),
      '#default_value' => $default_recipients,
    );
    $form['subject'] = array(
      '#type' => 'textfield',
      '#title' => t('Subject'),
      '#required' => TRUE,
    );
    $form['message'] = array(
      '#type' => 'textarea',
      '#title' => t('Message'),
      '#required' => TRUE,
    );
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Send e-mail'),
    );
    return $form;
  }
  // Not a valid group node.
  drupal_not_found();
}

/**
 * Submit handler; E-mail group members.
 */
function og_email_blast_group_submit($form, &$form_state) {
  og_email_blast_mail_send($form_state['values']);
  // Redirect to the group admin page.
  $form_state['redirect'] = $form_state['values']['entity_type'] . '/' . $form_state['values']['etid'] . '/group';
}

/**
 * Implements hook_mail_send().
 */
function og_email_blast_mail_send($form_values) {
  global $user;
  $module = 'og_email_blast';
  $key = 'group_message';
  $from = variable_get('site_mail', 'admin@example.com');

  $params = $form_values;
  $params['admin_email'] = $user->mail;
  $params['admin'] = format_username($user);
  $entities = entity_load($form_values['entity_type'], array($form_values['etid']));
  $entity = reset($entities);
  $params['group'] = entity_label($form_values['entity_type'], $entity);
  $roles = array();
  if (!empty($form_values['roles']) && $form_values['roles'] != 'all') {
    $roles = array($form_values['roles']);
  }

  $language = language_default();
  $emails = _group_emails($user, $form_values['entity_type'], $form_values['etid'], $roles);
  if (!empty($emails)) {
    $failed = array();
    foreach ($emails as $to) {
      $result = drupal_mail($module, $key, $to, $language, $params, $from);
      if (!$result['result'] && $result['send']) {
        $failed[] = $to;
      }
    }
    if (empty($failed)) {
      drupal_set_message(t('Your message has been sent.'));
    }
    else {
      drupal_set_message(t('There was an error when trying to send the mail to these recipients: %failed', array('%failed' => implode(', ', $emails))), 'error');
    }
  }
  else {
    drupal_set_message(t('There are not valid members to which a mail can be sent.'), 'warning');
  }
}

/**
 * Implements hook_mail().
 */
function og_email_blast_mail($key, &$message, $params) {
  switch ($key) {
    case 'group_message':
      $message['headers']['Reply-To'] = $params['admin_email'];
      $message['subject'] = check_plain($params['subject']);
      $message['body'][] = check_plain($params['message']);
      watchdog('og_email_blast', '@admin sent an e-mail to @address in the group @group', array('@admin' => $params['admin'], '@address' => $message['to'], '@group' => $params['group']));
      break;
  }
}

/**
 * Utility function to get group members' e-mail addresses. This list does not
 * include the group admin's e-mail address.
 *
 * @param $user
 *   The user object of the group admin requesting/using this info.
 *
 * @param $entity_type
 *   The group object's entity type.
 *
 * @param $etid
 *   The group object's entity id.
 *
 * @return
 *   A string of concatenated group members' e-mail addresses.
 */
function _group_emails($user, $entity_type, $etid, $roles = array()) {
  global $user;
  $query = db_select('og_membership', 'om')->fields('u', array('uid', 'mail'));
  $query->condition('om.gid', $etid)->condition('om.group_type', $entity_type)->condition('om.entity_type', 'user')->condition('om.state', 1);
  $query->innerJoin('users', 'u', 'u.uid = om.etid');
  $query->condition('u.uid', $user->uid, '<>');
  if (!empty($roles)) {
    $query->innerJoin('og_users_roles', 'our', 'our.gid = om.gid AND our.group_type = om.group_type AND our.uid = om.etid');
    $query->condition('our.rid', $roles);
  }
  $result = $query->execute()->fetchAllKeyed(0, 1);
  return $result;
}
