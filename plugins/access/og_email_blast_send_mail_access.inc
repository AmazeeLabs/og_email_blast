<?php


/**
 * @file
 * Plugin to provide access control based on the user access to send emails in
 * a group.
 */

/**
 * Plugins are described by creating a $plugin array which will be used
 * by the system that includes this file.
 */
$plugin = array(
  'title' => t("OG: send mails in group"),
  'description' => t('Controls the access based on the user being allowed to send mails in the group.'),
  'callback' => 'og_email_blast_send_mail_access_check',
  'settings form' => 'og_email_blast_send_mail_access_settings',
  'summary' => 'og_email_blast_send_mail_access_summary',
  'required context' => array(
    new ctools_context_required(t('User'), 'user'),
    new ctools_context_required(t('Node'), 'node'),
  ),
);

/**
 * Settings form for the 'by perm' access plugin
 */
function og_email_blast_send_mail_access_settings($form, &$form_state, $conf) {
  // For the moment, no other settings than the default ones.
  return $form;
}

/**
 * Check for access.
 */
function og_email_blast_send_mail_access_check($conf, $context) {
  // The next lines are taken from the og_perm.inc.
  // As far as I know there should always be a context at this point, but this
  // is safe.
  list($user_context, $node_context) = $context;
  if (empty($user_context) || empty($user_context->data)) {
    return;
  }

  if (empty($node_context) || empty($node_context->data)) {
    return;
  }

  $account = clone $user_context->data;
  $node = $node_context->data;

  return og_email_blast_mail_access('node', $node->nid, $account);
}

/**
 * Provide a summary description based upon the checked permissions.
 */
function og_email_blast_send_mail_access_summary($conf, $context) {
  list($user_context, $node_context) = $context;

  return t('@identifier can send mails in @group group"', array('@identifier' => $user_context->identifier, '@group' => $node_context->identifier));
}

