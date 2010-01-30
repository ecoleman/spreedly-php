<?php

include_once(dirname(__FILE__)."/setup.inc");

class SpreedlyTest extends PHPUnit_Framework_TestCase {
	public function testXmlParams() {
		$obj = new StdClass();
		$obj->first = "one";
		$obj->second = new StdClass();
		$obj->second->one = 123;
		$obj->second->two = 234;
		$obj->third = "three";
		$xml = Spreedly::__to_xml_params($obj);
		$this->assertEquals("<first>one</first><second><one>123</one><two>234</two></second><third>three</third>", $xml);
	}

	public function testWipe() {
		global $test_site_name, $test_token;
		Spreedly::configure($test_site_name, $test_token);

		SpreedlySubscriber::wipe();
		$sub = SpreedlySubscriber::create(1, "abel@nospam.com", "abel");
		$this->assertTrue($sub instanceof SpreedlySubscriber);

		SpreedlySubscriber::wipe();
		$sub = SpreedlySubscriber::find(1);
		$this->assertNull($sub);
	}

	public function testAdminUrl() {
		global $test_site_name, $test_token;
		Spreedly::configure($test_site_name, $test_token);
		$url = Spreedly::get_admin_subscriber_url(123);
		$this->assertEquals($url, "https://spreedly.com/{$test_site_name}/subscribers/123");
	}

	public function testConfigure() {
		global $test_site_name, $test_token;
		Spreedly::configure($test_site_name, $test_token);
		$this->assertNotNull(Spreedly::$token);
		$this->assertEquals(Spreedly::$token, $test_token);
		$this->assertEquals(Spreedly::$site_name, $test_site_name);
		$this->assertEquals(Spreedly::$base_uri, "https://spreedly.com/api/v4/{$test_site_name}");
	}

	public function testCreate() {
		global $test_site_name, $test_token;
		Spreedly::configure($test_site_name, $test_token);

		SpreedlySubscriber::wipe();
		$sub = SpreedlySubscriber::find(1);
		$this->assertNull($sub);

		$sub = SpreedlySubscriber::create(1, "abel@nospam.com", "abel");
		$sub = SpreedlySubscriber::find(1);
		$this->assertTrue($sub instanceof SpreedlySubscriber);
		$this->assertFalse($sub->active);
	}

	public function testDelete() {
		global $test_site_name, $test_token;
		Spreedly::configure($test_site_name, $test_token);

		SpreedlySubscriber::wipe();
		$sub = SpreedlySubscriber::find(1);
		$this->assertNull($sub);

		$sub = SpreedlySubscriber::create(1, "abel@nospam.com", "abel");
		$sub->comp(1, "days", "basic");
		$sub = SpreedlySubscriber::find(1);
		$this->assertTrue($sub instanceof SpreedlySubscriber);

		SpreedlySubscriber::delete(1);
		$sub = SpreedlySubscriber::find(1);
		$this->assertNull($sub);
	}

	public function testEditSubscriberUrl() {
		global $test_site_name, $test_token;
		Spreedly::configure($test_site_name, $test_token);
		$url = Spreedly::get_edit_subscriber_url("XYZ");
		$this->assertEquals("https://spreedly.com/{$test_site_name}/subscriber_accounts/XYZ", $url);
	}

	public function testFind() {
		global $test_site_name, $test_token;
		Spreedly::configure($test_site_name, $test_token);

		SpreedlySubscriber::wipe();
		$sub = SpreedlySubscriber::find(1);
		$this->assertNull($sub);

		$sub = SpreedlySubscriber::create(1, "abel@nospam.com", "abel");
		$sub = SpreedlySubscriber::create(2, "baker@nospam.com", "baker");
		$sub = SpreedlySubscriber::find(1);
		$this->assertTrue($sub instanceof SpreedlySubscriber);

		$sub = SpreedlySubscriber::find(2);
		$this->assertTrue($sub instanceof SpreedlySubscriber);
	}

	public function testFreeTrial() {
		global $test_site_name, $test_token;
		Spreedly::configure($test_site_name, $test_token);

		SpreedlySubscriber::wipe();
		$sub = SpreedlySubscriber::find(1);
		$this->assertNull($sub);

		$sub1 = SpreedlySubscriber::create(1, "abel@nospam.com", "abel");
		$sub1->comp(1, "days", "full");
		$sub2 = SpreedlySubscriber::create(2, "baker@nospam.com", "baker");

		$trial_plan = SpreedlySubscriptionPlan::find_by_name("Free Trial");
		$this->assertNotNull($trial_plan);
		try {
			$sub1->activate_free_trial($trial_plan->id);
			$this->fail("activated trial for existing customer");
		} catch (SpreedlyException $e) {
			// good
		}

		$sub2->activate_free_trial($trial_plan->id);
		$sub2 = SpreedlySubscriber::find($sub2->get_id());
		$this->assertNotNull($sub2);
		$this->assertTrue($sub2->on_trial);
	}

	public function testGetAll() {
		global $test_site_name, $test_token;
		Spreedly::configure($test_site_name, $test_token);

		SpreedlySubscriber::wipe();
		$sub = SpreedlySubscriber::find(1);
		$this->assertNull($sub);

		$sub = SpreedlySubscriber::create(1, "abel@nospam.com", "abel");
		$sub = SpreedlySubscriber::create(2, "baker@nospam.com", "baker");
		$sub = SpreedlySubscriber::create(3, "charlie@nospam.com", "charlie");
		$subs = SpreedlySubscriber::get_all();
		$this->assertEquals(3, count($subs));
	}

	public function testPlans() {
		global $test_site_name, $test_token;
		Spreedly::configure($test_site_name, $test_token);

		$plans = SpreedlySubscriptionPlan::get_all();
		$this->assertEquals(4, count($plans));
		$this->assertEquals("full", $plans[0]->feature_level);
		$this->assertEquals("basic", $plans[2]->feature_level);

		$full = SpreedlySubscriptionPlan::find_by_name("Annual");
		$this->assertNotNull($full);

		$id = $full->id;
		$full = SpreedlySubscriptionPlan::find($id);
		$this->assertNotNull($full);
	}

	public function testStopAutoRenew() {
		global $test_site_name, $test_token;
		Spreedly::configure($test_site_name, $test_token);

		SpreedlySubscriber::wipe();
		$sub = SpreedlySubscriber::create(75, null, "charlie");
		$sub->comp(32, "days", "basic");
		$sub = SpreedlySubscriber::find(75);
		$this->assertTrue($sub instanceof SpreedlySubscriber);
		$this->assertFalse($sub->recurring);

		$sub->stop_auto_renew();
		$sub = SpreedlySubscriber::find(75);
		$this->assertFalse($sub->recurring);
	}

	public function testSubscriberUrl() {
		global $test_site_name, $test_token;
		Spreedly::configure($test_site_name, $test_token);
		$url = Spreedly::get_subscribe_url(123, "full");
		$this->assertEquals("https://spreedly.com/{$test_site_name}/subscribers/123/subscribe/full/", $url);

		$url = Spreedly::get_subscribe_url(123, "full", "test_user");
		$this->assertEquals("https://spreedly.com/{$test_site_name}/subscribers/123/subscribe/full/test_user", $url);

		$url = Spreedly::get_subscribe_url(123, "full", "test/ user");
		$this->assertEquals("https://spreedly.com/{$test_site_name}/subscribers/123/subscribe/full/test%2F+user", $url);
	}

	public function testUpdate() {
		global $test_site_name, $test_token;
		Spreedly::configure($test_site_name, $test_token);

		SpreedlySubscriber::wipe();
		$sub = SpreedlySubscriber::create(75, null, "charlie");
		$this->assertEquals(75, $sub->get_id());

		$sub->update("test@test.com", "able");
		$sub = SpreedlySubscriber::find(75);
		$this->assertNotNull($sub);
		$this->assertEquals("test@test.com", $sub->email);
		$this->assertEquals("able", $sub->screen_name);

		$sub->update(null, null, 100);
		$sub = SpreedlySubscriber::find(75);
		$this->assertNull($sub);
		$sub = SpreedlySubscriber::find(100);
		$this->assertNotNull($sub);

		SpreedlySubscriber::create(75, null, "baker");
		try {
			$sub->update(null, null, 75);
			$this->fail("expected an exception to be thrown");
		} catch (SpreedlyException $e) {
		}
	}

	public function testLifetimeComp() {
		global $test_site_name, $test_token;
		Spreedly::configure($test_site_name, $test_token);
		SpreedlySubscriber::wipe();

		$sub = SpreedlySubscriber::create(75, null, "charlie");
		$this->assertEquals(75, $sub->get_id());
		$this->assertFalse($sub->lifetime_subscription);
		$sub = $sub->lifetime_comp("full");
		$this->assertTrue($sub->lifetime_subscription);
	}

	public function testAddStoreCredit() {
		global $test_site_name, $test_token;
		Spreedly::configure($test_site_name, $test_token);
		SpreedlySubscriber::wipe();

		$sub = SpreedlySubscriber::create(75, null, "charlie");
		$this->assertEquals(75, $sub->get_id());
		$this->assertEquals(0, $sub->store_credit);
		$sub->add_store_credit(2.50);
		$sub = SpreedlySubscriber::find(75);
		$this->assertEquals(2.50, $sub->store_credit);
	}

	public function testAddFees() {
		global $test_site_name, $test_token;
		Spreedly::configure($test_site_name, $test_token);
		SpreedlySubscriber::wipe();

		$sub = SpreedlySubscriber::create(75, null, "charlie");
		$sub->comp(1, "months", "full");
		$this->assertEquals(75, $sub->get_id());
		$sub->add_fee("Daily Bandwidth Charge", "313 MB used", "Traffic Fees", 2.34);
	}

	public function testAllowFreeTrial() {
		global $test_site_name, $test_token;
		Spreedly::configure($test_site_name, $test_token);
		SpreedlySubscriber::wipe();

		$trial_plan = SpreedlySubscriptionPlan::find_by_name("Free Trial");
		$sub = SpreedlySubscriber::create(75, null, "charlie");
		$sub->activate_free_trial($trial_plan->id);
		$sub = SpreedlySubscriber::find(75);
		$this->assertFalse($sub->eligible_for_free_trial);
		$sub->allow_free_trial();
		$sub = SpreedlySubscriber::find(75);
		$this->assertTrue($sub->eligible_for_free_trial);
	}

	public function testCreateInvoice() {
		global $test_site_name, $test_token;
		Spreedly::configure($test_site_name, $test_token);
		SpreedlySubscriber::wipe();

		// create invoice for existing customer
		$trial_plan = SpreedlySubscriptionPlan::find_by_name("Free Trial");
		$sub = SpreedlySubscriber::create(75, null, "charlie");
		$sub->activate_free_trial($trial_plan->id);
		$sub = SpreedlySubscriber::find(75);

		$annual = SpreedlySubscriptionPlan::find_by_name("Annual");
		$invoice = SpreedlyInvoice::create($sub->get_id(), $annual->id, $sub->screen_name, "test@test.com");

		$this->assertTrue($invoice->subscriber instanceof SpreedlySubscriber);
		$this->assertEquals("charlie", $invoice->subscriber->screen_name);
		$this->assertEquals(25, $invoice->line_items[0]->amount);

		// create invoice for new customer
		$monthly = SpreedlySubscriptionPlan::find_by_name("Monthly");
		$invoice = SpreedlyInvoice::create(10, $monthly->id, "able", "able@test.com");
		$this->assertTrue($invoice->subscriber instanceof SpreedlySubscriber);
		$this->assertEquals("able", $invoice->subscriber->screen_name);
		$this->assertEquals(2.50, $invoice->line_items[0]->amount);
		$this->assertTrue(SpreedlySubscriber::find(10) instanceof SpreedlySubscriber);
	}

	public function testPayInvoice() {
		global $test_site_name, $test_token;
		Spreedly::configure($test_site_name, $test_token);
		SpreedlySubscriber::wipe();

		// create invoice for existing customer
		$trial_plan = SpreedlySubscriptionPlan::find_by_name("Free Trial");
		$sub = SpreedlySubscriber::create(75, null, "charlie");
		$sub->activate_free_trial($trial_plan->id);
		$sub = SpreedlySubscriber::find(75);

		$annual = SpreedlySubscriptionPlan::find_by_name("Annual");
		$invoice = SpreedlyInvoice::create($sub->get_id(), $annual->id, $sub->screen_name, "test@test.com");
		$response = $invoice->pay("4222222222222", "visa", "123", "13", date("Y")+1, "Test", "User");
		$this->assertTrue($response instanceof SpreedlyErrorList);

		$response = $invoice->pay("4222222222222", "visa", "123", "12", date("Y")-1, "Test", "User");
		$this->assertTrue($response instanceof SpreedlyErrorList);

		// declined
		try {
			$response = $invoice->pay("4012888888881881", "visa", "123", "12", date("Y")+1, "Test", "User");
			$this->fail("An exception should have been thrown");
		} catch (SpreedlyException $e) {
			$this->assertEquals(403, $e->getCode());
		}

		$response = $invoice->pay("4222222222222", "visa", "123", "12", date("Y")+1, "Test", "User");
		$this->assertTrue($response->closed);

		// test paying paid invoice
		try {
			$response = $invoice->pay("4222222222222", "visa", "123", "12", date("Y")+1, "Test", "User");
			$this->fail("An exception should have been thrown");
		} catch (SpreedlyException $e) {
			$this->assertEquals(403, $e->getCode());
		}
	}
}
