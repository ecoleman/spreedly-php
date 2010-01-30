<?php
/**
 * Version 2.1
 * Copyright (c) 2009 Luke Ehresman
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

define("VERBOSE", false);

/**
 * Class: Spreedly
 *
 * Provides a convenient wrapper around the spreedly.com API.  Instead
 * of mucking around with HTTP you can just Spreedly::configure and
 * SpreedlySubscriber::find.  Much of the functionality is hung off
 * the SpreedlySubscriber class.
 *
 * By Luke Ehresman <luke@ehresman.org>
 * Based on the Spreedly Ruby gem by Nathaniel Talbott (v. 1.2.1)
 */
class Spreedly {
	static $token;
	static $site_name;
	static $base_uri;

	/**
	 * Method: configure
	 *
	 * Call this before you start using the API to set things up.
	 */
	public static function configure($site_name, $token) {
		self::$site_name = $site_name;
		self::$token = $token;
		self::$base_uri = "https://spreedly.com/api/v4/$site_name";
	}

	/**
	 * Method: get_edit_subscriber_url
	 *
	 * Generates an edit subscriber for the given subscriber token.  The
	 * token is returned with the subscriber (i.e. by SpreedlySubscriber::find)
	 */
	public static function get_edit_subscriber_url($token) {
		return "https://spreedly.com/".self::$site_name."/subscriber_accounts/$token";
	}

	/**
	 * Method: get_admin_subscriber_url
	 *
	 * Generates a link to the page on your Spreedly profile where you can
	 * administer a user.
	 */
	public static function get_admin_subscriber_url($id) {
		return "https://spreedly.com/".self::$site_name."/subscribers/$id";
	}

	/**
	 * Method: get_subscribe_url
	 *
	 * Generates a subscribe url for the given user id and plan.
	 */
	public static function get_subscribe_url($id, $plan, $screen_name=null) {
		$screen_name = ($screen_name ? urlencode($screen_name) : "");
		return "https://spreedly.com/".self::$site_name."/subscribers/$id/subscribe/$plan/$screen_name";
	}

	/**
	 * Method: get_transactions
	 *
	 * Returns an array of transaction data.
	 */
	public static function get_transactions($since_id=null) {
		$params = null;
		if (is_numeric($since_id))
			$params["since_id"] = $since_id;
		$result = Spreedly::__curl_request("/transactions.xml", "get", $params);
		print_r($result);
		return Spreedly::__parse_xml($result->response, "transaction", "StdClass");
	}

	/**
	 * Method: __curl_request
	 *
	 * Internal method used to make an HTTP request to Spreedly.
	 */
	public static function __curl_request($url, $method="get", $data=null) {
		$ch = curl_init(self::$base_uri.$url);
		if (VERBOSE) {
			echo "\n\n\n###############################################\n";
			curl_setopt($ch, CURLOPT_VERBOSE, true);
		}
		curl_setopt($ch, CURLOPT_FOLLOWLOCATION, false);
		curl_setopt($ch, CURLOPT_MAXREDIRS, 0);
		curl_setopt($ch, CURLOPT_RETURNTRANSFER, 1);
		curl_setopt($ch, CURLOPT_USERPWD, self::$token.":X");
		curl_setopt($ch, CURLOPT_SSL_VERIFYPEER, false);
		curl_setopt($ch, CURLOPT_SSL_VERIFYHOST, 2);
		curl_setopt($ch, CURLOPT_TIMEOUT, 8);
		curl_setopt($ch, CURLOPT_HTTPHEADER, array(
				"Content-Type: text/xml",
				"Accept: text/xml"
			));

		switch ($method) {
		case "post":
			if ($data) {
				curl_setopt($ch, CURLOPT_POST, true);
				curl_setopt($ch, CURLOPT_POSTFIELDS, $data);
			} else {
				curl_setopt($ch, CURLOPT_CUSTOMREQUEST, "POST");
			}
			break;
		case "delete":
			curl_setopt($ch, CURLOPT_CUSTOMREQUEST, "DELETE");
			break;
		case "put":
			$fh = fopen("php://memory", "rw");
			fwrite($fh, $data);
			rewind($fh);
			curl_setopt($ch, CURLOPT_INFILE, $fh);
			curl_setopt($ch, CURLOPT_INFILESIZE, strlen($data));
			curl_setopt($ch, CURLOPT_PUT, true);
			break;
		default:
			curl_setopt($ch, CURLOPT_CUSTOMREQUEST, "GET");
			break;
		}

		$result = new StdClass();
		$result->response = curl_exec($ch);
		$result->code = curl_getinfo($ch, CURLINFO_HTTP_CODE);
		return $result;
	}

	/**
	 * Method: __to_xml_params
	 *
	 * Converts an object structure into XML
	 */
	public static function __to_xml_params($hash, $change=true) {
		$result = "";
		foreach ($hash as $key=>$value) {
			$tag = $key;
			if ($change)
				$tag = str_replace("_", "-", $tag);
			$result .= "<$tag>";
			if (is_object($value))
				$result .= self::__to_xml_params($value, $change);
			else
				$result .= $value;
			$result .= "</$tag>";
		}
		return $result;
	}

	/**
	 * Method:: __parse_xml
	 *
	 * This will take an XML representation of a subscriber, or a list of
	 * subscribers, and create instances of the SpreedlySubscriber object.
	 * This will return either a single SpreedlySubscriber object, or
	 * an array of SpreedlySubscriber objects.
	 */
	public static function __parse_xml($xml, $node_name, $node_class) {
		if (is_string($xml))
			$dom = @DOMDocument::loadXML($xml);
		else
			$dom = $xml;
		if (!$dom) return null;

		$node_list = $dom->getElementsByTagName($node_name);
		$list = self::__parse_dom_node_list($node_list, $node_class);

		if (count($list) == 0)
			return null;
		else if (count($list) == 1)
			return $list[0];
		else
			return $list;
	}

	public static function __parse_dom_node_list($node_list, $node_class) {
		$list = array();
		for ($i=0; $i < $node_list->length; $i++) {
			if ($node_list->item($i) instanceof DOMElement) {
				$list[] = self::__parse_dom_node($node_list->item($i), $node_class);
			}
		}
		return $list;
	}

	public static function __parse_dom_node($node, $node_class) {
		$node = $node->firstChild;
		$obj = new $node_class();
		while ($node) {
			if ($node->nodeType == XML_ELEMENT_NODE) {
				$nodeName = str_replace("-", "_", $node->nodeName);
				if ($node->childNodes->length == 0 || ($node->childNodes->length == 1 && $node->firstChild->childNodes == 0)) {
					if (!is_numeric($node->nodeValue) && $tmp = strtotime($node->nodeValue)) {
						$obj->$nodeName = $tmp;
					} else if ($node->nodeValue == "false") {
						$obj->$nodeName = false;
					} else if ($node->nodeValue == "true") {
						$obj->$nodeName = true;
					} else {
						$obj->$nodeName = $node->nodeValue;
					}
				} else {
					$class = "StdClass";
					if ($nodeName == "subscriber")
						$class = "SpreedlySubscriber";
					if ($node->hasAttribute("type") && $node->getAttribute("type") == "array") {
						$obj->$nodeName = self::__parse_dom_node_list($node->childNodes, $class);
					} else {
						$obj->$nodeName = self::__parse_dom_node($node, $class);
					}
				}
			}
			$node = $node->nextSibling;
		}
		return $obj;
	}
}

/**
 * Class: SpreedlySubscriber
 */
class SpreedlySubscriber {
	/**
	 * Method: __construct
	 */
	public function __construct() {
	}

	/**
	 * Method: activate_free_trial
	 *
	 * Activates a free trial on the subscriber.  Requires subscription_id
	 * of the free trial plan.
	 */
	public function activate_free_trial($subscription_id) {
		$obj = new StdClass();
		$obj->subscription_plan = new StdClass();
		$obj->subscription_plan->id = $subscription_id;
		$xml = Spreedly::__to_xml_params($obj);
		$result = Spreedly::__curl_request("/subscribers/{$this->customer_id}/subscribe_to_free_trial.xml", "post", $xml);

		if (preg_match("/^2..$/", $result->code)) {
			return Spreedly::__parse_xml($result->response, "subscriber", "SpreedlySubscriber");
		} else {
			throw new SpreedlyException("Could not activate free trial for subscriber: {$result->response} ({$result->code})", $result->code);
		}
	}

	/**
	 * Method: stop_auto_renew
	 */
	public function stop_auto_renew() {
		$result = Spreedly::__curl_request("/subscribers/{$this->customer_id}/stop_auto_renew.xml", "post");
		if (preg_match("/^2..$/", $result->code)) {
			return null;
		} else {
			throw new SpreedlyException("Could not stop auto renew for subscriber: {$result->response} ({$result->code})", $result->code);
		}
	}

	/**
	 * Method: comp
	 *
	 * Allows you to give a complimentary subscription (if the
	 * subscriber is inactive) or a complimentary time extension (if
	 * the subscriber is active).  Automatically figures out which to
	 * do.  Note: units must be one of "days" or "months" (Spreedly
	 * enforced).
	 */
	public function comp($quantity, $units, $feature_level=null) {
		$type = $this->active ? "complimentary_time_extensions" : "complimentary_subscriptions";
		if ($type == "complimentary_subscriptions" && !$feature_level)
			throw new SpreedlyException("Feature level can't be blank when comping an inactive subscription");

		$node_name = substr($type, 0, -1);
		$obj = new StdClass();
		$obj->$node_name = new StdClass();
		$obj->$node_name->duration_quantity = $quantity;
		$obj->$node_name->duration_units = $units;
		if ($feature_level)
			$obj->$node_name->feature_level = $feature_level;

		$xml = Spreedly::__to_xml_params($obj);
		$result = Spreedly::__curl_request("/subscribers/{$this->customer_id}/$type.xml", "post", $xml);

		if (preg_match("/^2..$/", $result->code)) {
			return Spreedly::__parse_xml($result->response, "subscriber", "SpreedlySubscriber");
		} else {
			throw new SpreedlyException("Could not comp subscriber: {$result->response} ({$result->code})", $result->code);
		}
	}

	/**
	 * Method: update
	 *
	 * Updates a subscriber's details on Spreedly.
	 */
	public function update($email=null, $screen_name=null, $new_customer_id=null) {
		$obj = new StdClass();
		$obj->subscriber = new StdClass();
		if ($email)
			$obj->subscriber->email = $email;
		if ($screen_name)
			$obj->subscriber->screen_name = $screen_name;
		if ($new_customer_id)
			$obj->subscriber->new_customer_id = $new_customer_id;
		$xml = Spreedly::__to_xml_params($obj);
		$result = Spreedly::__curl_request("/subscribers/{$this->get_id()}.xml", "put", $xml);

		if (preg_match("/^2..$/", $result->code)) {
			return Spreedly::__parse_xml($result->response, "subscriber", "SpreedlySubscriber");
		} else if ($result->code == 403) {
			throw new SpreedlyException("Could not update subscriber: new customer_id already exists.", $result->code);
		} else {
			throw new SpreedlyException("Could not update subscriber: {$result->response} ({$result->code})", $result->code);
		}
	}

	/**
	 * Method: lifetime_comp
	 *
	 * Adds a complimentary lifetime subscription to the subscriber
	 */
	public function lifetime_comp($feature_level) {
		$obj = new StdClass();
		$obj->lifetime_complimentary_subscription = new StdClass();
		$obj->lifetime_complimentary_subscription->feature_level = $feature_level;
		$xml = Spreedly::__to_xml_params($obj);
		$result = Spreedly::__curl_request("/subscribers/{$this->get_id()}/lifetime_complimentary_subscriptions.xml", "post", $xml);

		if (preg_match("/^2..$/", $result->code)) {
			return Spreedly::__parse_xml($result->response, "subscriber", "SpreedlySubscriber");
		} else {
			throw new SpreedlyException("Could not add lifetime comp to subscriber: {$result->response} ({$result->code})", $result->code);
		}
	}

	/**
	 * Method: add_store_credit
	 *
	 * Adds store credit to the subscriber
	 */
	public function add_store_credit($amount) {
		$obj = new StdClass();
		$obj->credit = new StdClass();
		$obj->credit->amount = $amount;
		$xml = Spreedly::__to_xml_params($obj);
		$result = Spreedly::__curl_request("/subscribers/{$this->get_id()}/credits.xml", "post", $xml);

		if (preg_match("/^2..$/", $result->code)) {
			return Spreedly::__parse_xml($result->response, "subscriber", "SpreedlySubscriber");
		} else {
			throw new SpreedlyException("Could not add store credit to subscriber: {$result->response} ({$result->code})", $result->code);
		}
	}

	/**
	 * Method: add_fee
	 *
	 * adds a fee to the subscriber
	 */
	public function add_fee($name, $description, $group, $amount) {
		$obj = new StdClass();
		$obj->fee = new StdClass();
		$obj->fee->name = $name;
		$obj->fee->description = $description;
		$obj->fee->group = $group;
		$obj->fee->amount = $amount;
		$xml = Spreedly::__to_xml_params($obj);
		$result = Spreedly::__curl_request("/subscribers/{$this->get_id()}/fees.xml", "post", $xml);

		if (preg_match("/^2..$/", $result->code)) {
			return Spreedly::__parse_xml($result->response, "subscriber", "SpreedlySubscriber");
		} else {
			throw new SpreedlyException("Could not add fees to subscriber: {$result->response} ({$result->code})", $result->code);
		}
	}

	/**
	 * Method: allow_free_trial
	 *
	 * allow the subscriber to have another free trial
	 */
	public function allow_free_trial() {
		$result = Spreedly::__curl_request("/subscribers/{$this->customer_id}/allow_free_trial.xml", "post");
		if (preg_match("/^2..$/", $result->code)) {
			return null;
		} else {
			throw new SpreedlyException("Could not allow free trial for subscriber: {$result->response} ({$result->code})", $result->code);
		}
	}

	/**
	 * Method: get_id
	 *
	 * Spreedly calls your id for the user the "customer id".  This gives
	 * you a handy alias so you can just call it "id".
	 */
	public function get_id() {
		return $this->customer_id;
	}


	/******************************************************************/
	/**  PUBLIC STATIC METHODS                                       **/
	/******************************************************************/

	/**
	 * Method: get_all
	 *
	 * Returns all the subscribers in your site.
	 */
	public static function get_all() {
		$result = Spreedly::__curl_request("/subscribers.xml", "get");
		if (preg_match("/^2..$/", $result->code)) {
			return Spreedly::__parse_xml($result->response, "subscriber", "SpreedlySubscriber");
		} else if ($result->code == 404) {
			return null;
		} else {
			throw new SpreedlyException("Could not get all subscribers: {$result->response} ({$result->code})", $result->code);
		}
	}

	/**
	 * Method: create
	 *
	 * Creates a new subscriber on Spreedly.  The subscriber will NOT be
	 * active - they have to pay or you have to comp them for that to
	 * happen.
	 */
	public static function create($id, $email=null, $screen_name=null) {
		$obj = new StdClass();
		$obj->subscriber = new StdClass();
		$obj->subscriber->customer_id = $id;
		$obj->subscriber->email = $email;
		$obj->subscriber->screen_name = $screen_name;
		$xml = Spreedly::__to_xml_params($obj);
		$result = Spreedly::__curl_request("/subscribers.xml", "post", $xml);

		if (preg_match("/^2..$/", $result->code)) {
			return Spreedly::__parse_xml($result->response, "subscriber", "SpreedlySubscriber");
		} else if ($result->code == 403) {
			throw new SpreedlyException("Could not create subscriber: no id passed OR already exists.", $result->code);
		} else {
			throw new SpreedlyException("Could not create subscriber: {$result->response} ({$result->code})", $result->code);
		}
	}

	/**
	 * Method: delete
	 *
	 * This will DELETE individual subscribers from the site.  Pass in the
	 * customer_id.  Only works for test sites (enforced on the Spreedly
	 * side).
	 */
	public static function delete($id) {
		Spreedly::__curl_request("/subscribers/{$id}.xml", "delete");
	}

	/**
	 * Method: find
	 *
	 * Looks up a subscriber by id.
	 */
	public static function find($id) {
		$result = Spreedly::__curl_request("/subscribers/{$id}.xml", "get");
		if (preg_match("/^2..$/", $result->code)) {
			return Spreedly::__parse_xml($result->response, "subscriber", "SpreedlySubscriber");
		} else if ($result->code == 404) {
			return null;
		} else {
			throw new SpreedlyException("Could not find subscriber: {$result->response} ({$result->code})", $result->code);
		}
	}

	/**
	 * Method: wipe
	 *
	 * This will DELETE all the subscribers from the site.  Only works for
	 * test sites (enforced on the Spreedly side).
	 */
	public static function wipe() {
		Spreedly::__curl_request("/subscribers.xml", "delete");
	}
}

/**
 * Class: SpreedlySubscriptionPlan
 */
class SpreedlySubscriptionPlan {
	/**
	 * Method: is_trial
	 *
	 * Convenience method for determining if this plan is a free trial plan
	 * or not.
	 */
	public function is_trial() {
		return $this->plan_type == "free_trial";
	}


	/**
	 * Method: get_all
	 *
	 * Returns all the subscription plans defined in your site.
	 */
	public static function get_all() {
		$result = Spreedly::__curl_request("/subscription_plans.xml", "get");
		if (preg_match("/^2..$/", $result->code)) {
			$list = Spreedly::__parse_xml($result->response, "subscription-plan", "SpreedlySubscriptionPlan");
			if (!is_array($list))
				$list = array($list);
			return $list;
		} else {
			throw new SpreedlyException("Could not get subscription plans: {$result->response} ({$result->code})", $result->code);
		}
	}

	/**
	 * Method: find
	 *
	 * Returns the subscription plan with the given id.
	 */
	public static function find($id) {
		$all = self::get_all();
		foreach ($all as $plan)
			if ($plan->id == $id)
				return $plan;
		return null;
	}

	/**
	 * Method: find_by_name
	 *
	 * Returns the first subscription plan with the given name.
	 */
	public static function find_by_name($name) {
		$all = self::get_all();
		foreach ($all as $plan)
			if (strtolower($plan->name) == strtolower($name))
				return $plan;
		return null;
	}
}

/**
 * Class: SpreedlyInvoice
 */
class SpreedlyInvoice {
	public static function create($customer_id, $subscription_id, $screen_name=null, $email=null) {
		$obj = new StdClass();
		$obj->invoice = new StdClass();
		$obj->invoice->subscription_plan_id = $subscription_id;
		$obj->invoice->subscriber = new StdClass();
		$obj->invoice->subscriber->customer_id = $customer_id;
		$obj->invoice->subscriber->screen_name = $screen_name;
		$obj->invoice->subscriber->email = $email;
		$xml = Spreedly::__to_xml_params($obj);
		$result = Spreedly::__curl_request("/invoices.xml", "post", $xml);

		if (preg_match("/^2..$/", $result->code)) {
			return Spreedly::__parse_xml($result->response, "invoice", "SpreedlyInvoice");
		} else {
			throw new SpreedlyException("Could not create invoice: {$result->response} ({$result->code})", $result->code);
		}
	}

	public function pay($card_number, $card_type, $verification_value, $month, $year, $first_name, $last_name) {
		$obj = new StdClass();
		$obj->payment = new StdClass();
		$obj->payment->credit_card = new StdClass();
		$obj->payment->credit_card->number = $card_number;
		$obj->payment->credit_card->card_type = $card_type;
		$obj->payment->credit_card->verification_value = $verification_value;
		$obj->payment->credit_card->month = $month;
		$obj->payment->credit_card->year = $year;
		$obj->payment->credit_card->first_name = $first_name;
		$obj->payment->credit_card->last_name = $last_name;
		$xml = Spreedly::__to_xml_params($obj);
		$result = Spreedly::__curl_request("/invoices/{$this->token}/pay.xml", "put", $xml);

		if ($result->code == 404) {
			throw new SpreedlyException("The specified invoice could not be found.", $result->code);
		} else if ($result->code == 422) {
			return new SpreedlyErrorList(Spreedly::__parse_xml($result->response, "errors", "StdClass"));
		} else if ($result->code == 504) {
			throw new SpreedlyException("Gateway timeout.", $result->code);
		} else if (preg_match("/^2..$/", $result->code)) {
			// success!
			return Spreedly::__parse_xml($result->response, "invoice", "SpreedlyInvoice");
		} else {
			throw new SpreedlyException("Could not pay invoice: {$result->response} ({$result->code})", $result->code);
		}
	}
}


/**
 * Class: SpreedlyException
 */
class SpreedlyException extends Exception {
}

/**
 * Class: SpreedlyErrorList
 */
class SpreedlyErrorList {
	private $errors = array();

	public function __construct($list) {
		if ($list->error)
			$this->errors[] = $list->error;
		else
			$this->errors = $list;
	}

	public function get_errors() {
		return $errors;
	}
}
