package net.satka.bleManager.ui

import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.util.Log
import android.view.MenuItem
import android.view.View
import androidx.activity.enableEdgeToEdge
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.LinearLayoutManager
import com.google.android.material.snackbar.Snackbar
import com.journeyapps.barcodescanner.ScanContract
import com.journeyapps.barcodescanner.ScanIntentResult
import com.journeyapps.barcodescanner.ScanOptions
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import net.satka.bleManager.R
import net.satka.bleManager.ble.services.BluetoothDiscoveryService
import net.satka.bleManager.data.db.AppDatabase
import net.satka.bleManager.data.model.Device
import net.satka.bleManager.databinding.ActivityAddNewDeviceBinding
import net.satka.bleManager.ui.adapters.UnknownBluetoothDeviceAdapter
import net.satka.bleManager.ui.models.UnknownBluetoothDeviceModel
import net.satka.bleManager.utils.DebouncedVisibilitySetter
import net.satka.bleManager.utils.InsetsUtil

class AddNewDeviceActivity : AppCompatActivity() {
    companion object {
        private val CLASS_NAME = AddNewDeviceActivity::class.java.name

        // QR code formats:
        //   blemanager:?mac=AA:BB:CC:DD:EE:FF&name=Pump%203
        //   blemanager:?name=SCHIER-00123
        //   AA:BB:CC:DD:EE:FF
        private const val QR_SCHEME_PREFIX = "blemanager:"
        private const val QR_NAME_SEARCH_TIMEOUT_MILLIS = 30_000L
    }

    private lateinit var bluetoothDiscoveryService: BluetoothDiscoveryService
    private lateinit var database: AppDatabase
    private lateinit var debouncedVisibilitySetter: DebouncedVisibilitySetter
    private lateinit var binding: ActivityAddNewDeviceBinding
    private val devicesList = mutableListOf<UnknownBluetoothDeviceModel>()

    private val unknownBluetoothDeviceAdapter = UnknownBluetoothDeviceAdapter(devicesList)

    // Device name from a scanned QR code that we are waiting for discovery to find
    private var pendingQrName: String? = null
    private var pendingQrSnackbar: Snackbar? = null
    private var pendingQrTimeout: Job? = null

    private val qrScanLauncher = registerForActivityResult(ScanContract(), ::onQrScanned)

    private fun startDiscovery() {
        bluetoothDiscoveryService.setIsActive(true)
    }

    private fun stopDiscovery() {
        bluetoothDiscoveryService.setIsActive(false)
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        window.isNavigationBarContrastEnforced = false
        enableEdgeToEdge()

        binding = ActivityAddNewDeviceBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.topAppBar.setNavigationOnClickListener(::onToolbarNavigationBackClick)
        binding.topAppBar.setOnMenuItemClickListener(::onMenuItemClick)

        binding.recyclerViewBluetoothDevices.setOnApplyWindowInsetsListener(InsetsUtil::applyWindowsInsets)
        binding.recyclerViewBluetoothDevices.layoutManager = LinearLayoutManager(this)
        binding.recyclerViewBluetoothDevices.adapter = unknownBluetoothDeviceAdapter
        unknownBluetoothDeviceAdapter.onItemClick = ::onDeviceSelected

        debouncedVisibilitySetter = DebouncedVisibilitySetter(1000, binding.progressIndicator)
        bluetoothDiscoveryService = BluetoothDiscoveryService(this)
        bluetoothDiscoveryService.onDiscoveryStateChanged = debouncedVisibilitySetter::setIsVisible
        bluetoothDiscoveryService.onDeviceFound = ::onDeviceFound

        database = AppDatabase.getDatabase(this)
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        bluetoothDiscoveryService.onRequestPermissionsResult(requestCode, grantResults)
    }

    private fun onToolbarNavigationBackClick(view: View) {
        finish()
    }

    private fun onMenuItemClick(item: MenuItem): Boolean {
        return when (item.itemId) {
            R.id.scan_qr -> {
                cancelPendingQrSearch()
                qrScanLauncher.launch(
                    ScanOptions()
                        .setDesiredBarcodeFormats(ScanOptions.QR_CODE)
                        .setPrompt(getString(R.string.scan_qr_prompt))
                        .setBeepEnabled(false)
                        .setOrientationLocked(false)
                )
                true
            }

            else -> false
        }
    }

    private fun onQrScanned(result: ScanIntentResult) {
        // null when the user cancelled the scanner
        val contents = result.contents?.trim() ?: return

        val mac: String?
        val name: String?
        if (contents.startsWith(QR_SCHEME_PREFIX, ignoreCase = true)) {
            // "blemanager:?..." is an opaque URI, so parse just the query part
            val query = Uri.Builder().encodedQuery(contents.substringAfter('?', "")).build()
            mac = query.getQueryParameter("mac")
            name = query.getQueryParameter("name")?.takeIf { it.isNotBlank() }
        } else {
            mac = contents
            name = null
        }

        val normalizedMac = mac?.uppercase()?.replace('-', ':')
        when {
            normalizedMac != null && BluetoothAdapter.checkBluetoothAddress(normalizedMac) -> {
                stopDiscovery()
                addToKnownDevices(normalizedMac, name)
            }

            mac == null && name != null -> startQrNameSearch(name)

            else -> Snackbar.make(binding.root, R.string.invalid_qr_code, Snackbar.LENGTH_LONG)
                .show()
        }
    }

    private fun startQrNameSearch(name: String) {
        // The device may already have been found before the QR code was scanned
        val alreadyFound = devicesList.firstOrNull { it.name == name }
        if (alreadyFound != null) {
            onDeviceSelected(alreadyFound, devicesList.indexOf(alreadyFound))
            return
        }

        pendingQrName = name
        pendingQrSnackbar = Snackbar.make(
            binding.root,
            getString(R.string.looking_for_device, name),
            Snackbar.LENGTH_INDEFINITE
        ).setAction(R.string.cancel) { cancelPendingQrSearch() }
            .also { it.show() }
        pendingQrTimeout = lifecycleScope.launch {
            delay(QR_NAME_SEARCH_TIMEOUT_MILLIS)
            Snackbar.make(
                binding.root,
                getString(R.string.device_not_found, name),
                Snackbar.LENGTH_LONG
            ).show()
            cancelPendingQrSearch()
        }
    }

    private fun cancelPendingQrSearch() {
        pendingQrName = null
        pendingQrTimeout?.cancel()
        pendingQrTimeout = null
        pendingQrSnackbar?.dismiss()
        pendingQrSnackbar = null
    }

    private fun addToKnownDevices(macAddress: String, name: String?) {
        cancelPendingQrSearch()
        val context = this
        CoroutineScope(Dispatchers.IO).launch {
            // A scanned QR code can point to an already known device - keep its settings
            val knownDevice = database.deviceDao().getDeviceByMac(macAddress)
            val deviceName = knownDevice?.name ?: name ?: macAddress
            if (knownDevice == null) {
                database.deviceDao().insertDevice(
                    Device(
                        macAddress, deviceName,
                        getString(R.string.default_descriptor_uuid_mask),
                        false
                    )
                )
            }

            withContext(Dispatchers.Main) {
                val intent = Intent(context, DeviceDetailActivity::class.java)
                intent.putExtra(resources.getString(R.string.key_devicename), deviceName)
                intent.putExtra(resources.getString(R.string.key_deviceaddress), macAddress)
                startActivity(intent)
                finish()
            }
        }
    }

    private fun onDeviceFound(device: BluetoothDevice) {
        var deviceName: String? = getString(R.string.no_permissions)

        try {
            deviceName = device.name
        } catch (ex: SecurityException) {
            Log.e(CLASS_NAME, "exception", ex)
        }

        val deviceAddress = device.address
        if (deviceAddress != null && pendingQrName != null && deviceName == pendingQrName) {
            stopDiscovery()
            addToKnownDevices(deviceAddress, deviceName)
            return
        }

        if (deviceAddress != null) {
            CoroutineScope(Dispatchers.IO).launch {
                if (database.deviceDao().getDeviceByMac(deviceAddress) == null) {
                    withContext(Dispatchers.Main) {
                        val deviceIndex =
                            devicesList.indexOfFirst { it.address == deviceAddress }
                        if (deviceIndex == -1) {
                            devicesList.add(
                                UnknownBluetoothDeviceModel(
                                    deviceName, deviceAddress
                                )
                            )
                            unknownBluetoothDeviceAdapter.notifyItemInserted(devicesList.size - 1)
                        }
                    }
                }
            }
        }
    }

    private fun onDeviceSelected(device: UnknownBluetoothDeviceModel, position: Int) {
        //Pouze jeden click, pak přecházíme do jiné agendy
        unknownBluetoothDeviceAdapter.onItemClick = null
        stopDiscovery()
        addToKnownDevices(device.address, device.name)
    }

    override fun onResume() {
        super.onResume()
        startDiscovery()
    }

    override fun onPause() {
        super.onPause()
        stopDiscovery()
    }

    override fun onDestroy() {
        super.onDestroy()
        bluetoothDiscoveryService.destroy()
    }
}