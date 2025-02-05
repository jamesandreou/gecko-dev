"use strict";

var {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

Cu.import("resource://gre/modules/ExtensionUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "AddonManager",
                                  "resource://gre/modules/AddonManager.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "ExtensionManagement",
                                  "resource://gre/modules/ExtensionManagement.jsm");

var {
  EventManager,
  SingletonEventManager,
  ignoreEvent,
} = ExtensionUtils;

XPCOMUtils.defineLazyModuleGetter(this, "NativeApp",
                                  "resource://gre/modules/NativeMessaging.jsm");

extensions.registerSchemaAPI("runtime", context => {
  let {extension} = context;
  return {
    runtime: {
      onStartup: new EventManager(context, "runtime.onStartup", fire => {
        extension.onStartup = fire;
        return () => {
          extension.onStartup = null;
        };
      }).api(),

      onInstalled: ignoreEvent(context, "runtime.onInstalled"),

      onMessage: context.messenger.onMessage("runtime.onMessage"),

      onConnect: context.messenger.onConnect("runtime.onConnect"),

      onUpdateAvailable: new SingletonEventManager(context, "runtime.onUpdateAvailable", fire => {
        let instanceID = extension.addonData.instanceID;
        AddonManager.addUpgradeListener(instanceID, upgrade => {
          extension.upgrade = upgrade;
          let details = {
            version: upgrade.version,
          };
          context.runSafe(fire, details);
        });
        return () => {
          AddonManager.removeUpgradeListener(instanceID);
        };
      }).api(),

      reload: () => {
        if (extension.upgrade) {
          // If there is a pending update, install it now.
          extension.upgrade.install();
        } else {
          // Otherwise, reload the current extension.
          AddonManager.getAddonByID(extension.id, addon => {
            addon.reload();
          });
        }
      },

      connect: function(extensionId, connectInfo) {
        let name = connectInfo !== null && connectInfo.name || "";
        extensionId = extensionId || extension.id;
        let recipient = {extensionId};

        return context.messenger.connect(Services.cpmm, name, recipient);
      },

      sendMessage: function(...args) {
        let options; // eslint-disable-line no-unused-vars
        let extensionId, message, responseCallback;
        if (typeof args[args.length - 1] == "function") {
          responseCallback = args.pop();
        }
        if (!args.length) {
          return Promise.reject({message: "runtime.sendMessage's message argument is missing"});
        } else if (args.length == 1) {
          message = args[0];
        } else if (args.length == 2) {
          if (typeof args[0] == "string" && args[0]) {
            [extensionId, message] = args;
          } else {
            [message, options] = args;
          }
        } else if (args.length == 3) {
          [extensionId, message, options] = args;
        } else if (args.length == 4 && !responseCallback) {
          return Promise.reject({message: "runtime.sendMessage's last argument is not a function"});
        } else {
          return Promise.reject({message: "runtime.sendMessage received too many arguments"});
        }

        if (extensionId != null && typeof extensionId != "string") {
          return Promise.reject({message: "runtime.sendMessage's extensionId argument is invalid"});
        }
        if (options != null && typeof options != "object") {
          return Promise.reject({message: "runtime.sendMessage's options argument is invalid"});
        }
        // TODO(robwu): Validate option keys and values when we support it.

        extensionId = extensionId || extension.id;
        let recipient = {extensionId};

        return context.messenger.sendMessage(Services.cpmm, message, recipient, responseCallback);
      },

      connectNative(application) {
        let app = new NativeApp(extension, context, application);
        return app.portAPI();
      },

      sendNativeMessage(application, message) {
        let app = new NativeApp(extension, context, application);
        return app.sendMessage(message);
      },

      get lastError() {
        return context.lastError;
      },

      getManifest() {
        return Cu.cloneInto(extension.manifest, context.cloneScope);
      },

      id: extension.id,

      getURL: function(url) {
        return extension.baseURI.resolve(url);
      },

      getPlatformInfo: function() {
        return Promise.resolve(ExtensionUtils.PlatformInfo);
      },

      openOptionsPage: function() {
        if (!extension.manifest.options_ui) {
          return Promise.reject({message: "No `options_ui` declared"});
        }

        return openOptionsPage(extension).then(() => {});
      },

      setUninstallURL: function(url) {
        if (url.length == 0) {
          return Promise.resolve();
        }

        let uri;
        try {
          uri = NetUtil.newURI(url);
        } catch (e) {
          return Promise.reject({message: `Invalid URL: ${JSON.stringify(url)}`});
        }

        if (uri.scheme != "http" && uri.scheme != "https") {
          return Promise.reject({message: "url must have the scheme http or https"});
        }

        extension.uninstallURL = url;
        return Promise.resolve();
      },
    },
  };
});
