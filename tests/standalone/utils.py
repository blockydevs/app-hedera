from pathlib import Path
from ragger.navigator import NavInsID
from ledgered.devices import Devices

ROOT_SCREENSHOT_PATH = Path(__file__).parent.resolve()

def navigation_helper_confirm(firmware, scenario_navigator):
    device_info = Devices.get_by_type(firmware)
    if device_info.name.startswith("nano"):
        scenario_navigator.review_approve(custom_screen_text="Confirm")
    else:
        scenario_navigator.review_approve()

def navigation_helper_reject(firmware, scenario_navigator):
    device_info = Devices.get_by_type(firmware)
    if device_info.name == "nanos":
        scenario_navigator.review_reject(custom_screen_text="Deny")
    elif device_info.name.startswith("nano"):
        scenario_navigator.review_reject(custom_screen_text="Reject")
    else:
        scenario_navigator.review_reject()


def navigate_erc20_confirm(firmware, navigator, scenario_navigator, screenshots_path: Path, test_name: str):
    print(type(firmware))
    """Standard ERC20 navigation schema: capture NBGL warning, then approve review."""
    device_info = Devices.get_by_type(firmware)
    if device_info.name.startswith("nano"):
        print("navigate_erc20_confirm nano")
        navigator.navigate_and_compare(
            screenshots_path,
            f"{test_name}_warning",
            [NavInsID.RIGHT_CLICK, NavInsID.RIGHT_CLICK, NavInsID.RIGHT_CLICK, NavInsID.RIGHT_CLICK],
            screen_change_after_last_instruction=False,
        )
        scenario_navigator.review_approve(custom_screen_text="Confirm")
    else:
        navigator.navigate_and_compare(
            screenshots_path,
            f"{test_name}_warning",
            [NavInsID.USE_CASE_CHOICE_CONFIRM],
            screen_change_after_last_instruction=False,
        )
        scenario_navigator.review_approve(test_name=test_name)


def navigate_erc20_reject(firmware, navigator, scenario_navigator, screenshots_path: Path, test_name: str):
    """Standard ERC20 navigation schema: capture NBGL warning, then reject review."""
    device_info = Devices.get_by_type(firmware)
    if device_info.name.startswith("nano"):
        print("navigate_erc20_reject nano")
        navigator.navigate_and_compare(
            screenshots_path,
            f"{test_name}_warning",
            [NavInsID.RIGHT_CLICK, NavInsID.RIGHT_CLICK, NavInsID.RIGHT_CLICK, NavInsID.RIGHT_CLICK],
            screen_change_after_last_instruction=False,
        )
        scenario_navigator.review_reject(custom_screen_text="Reject")
    else:
        navigator.navigate_and_compare(
            screenshots_path,
            f"{test_name}_warning",
            [NavInsID.USE_CASE_CHOICE_CONFIRM],
            screen_change_after_last_instruction=False,
        )
        scenario_navigator.review_reject(test_name=test_name)

def navigate_erc20_reject_at_warning(firmware, navigator, scenario_navigator, screenshots_path: Path, test_name: str):
    """Standard ERC20 navigation schema: capture NBGL warning, then reject review."""
    device_info = Devices.get_by_type(firmware)
    if device_info.name.startswith("nano"):
        print("navigate_erc20_reject_at_warning nano")
        navigator.navigate_and_compare(
            screenshots_path,
            f"{test_name}_warning",
            [NavInsID.RIGHT_CLICK, NavInsID.RIGHT_CLICK, NavInsID.RIGHT_CLICK, NavInsID.RIGHT_CLICK],
            screen_change_after_last_instruction=False,
        )
        scenario_navigator.review_reject(custom_screen_text="Reject")
    else:
        navigator.navigate_and_compare(
            screenshots_path,
            f"{test_name}_warning",
            [NavInsID.USE_CASE_CHOICE_REJECT],
            screen_change_after_last_instruction=True,
        )

def navigate_erc20_show_qr_code(firmware, navigator, scenario_navigator, screenshots_path: Path, test_name: str):
    """Standard ERC20 navigation schema: capture NBGL warning, then show QR code."""
    device_info = Devices.get_by_type(firmware)
    if device_info.name.startswith("nano"):
        raise NotImplementedError("navigate_erc20_show_qr_code is not implemented for nano")
    else:
        navigator.navigate_and_compare(
            screenshots_path,
            f"{test_name}_qr",
            [NavInsID.RIGHT_HEADER_TAP, NavInsID.LEFT_HEADER_TAP, NavInsID.USE_CASE_CHOICE_CONFIRM],
            screen_change_after_last_instruction=False,
        )
        scenario_navigator.review_approve(test_name=test_name)