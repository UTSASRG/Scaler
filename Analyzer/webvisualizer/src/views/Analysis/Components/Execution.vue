<template>
  <v-container>
    <v-row>
      <v-col sm="12" md="5">
        <v-list subheader three-line>
          <v-subheader class="text-h4">Environment</v-subheader>

          <v-list-item v-for="item in executionItems" :key="item.key">
            <v-list-item-content>
              <v-list-item-title>{{ item.key }}</v-list-item-title>
              <v-list-item-subtitle>{{ item.val }}</v-list-item-subtitle>
            </v-list-item-content>
            <v-list-item-action v-if="item.extra">
              <v-btn icon @click="showDialog(item.key, item.extra, true)">
                <v-icon color="grey lighten-1">mdi-information</v-icon>
              </v-btn>
            </v-list-item-action>
          </v-list-item>
        </v-list>
      </v-col>
      <v-col ms="12" md="5">
        <v-list subheader three-line>
          <v-subheader class="text-h4">Config</v-subheader>

          <v-list-item v-for="item in configItems" :key="item.key">
            <v-list-item-content>
              <v-list-item-title>{{ item.key }}</v-list-item-title>
              <v-list-item-subtitle>{{ item.val }}</v-list-item-subtitle>
            </v-list-item-content>
            <v-list-item-action v-if="item.extra">
              <v-btn icon @click="showDialog(item.key, item.extra, true)">
                <v-icon color="grey lighten-1">mdi-information</v-icon>
              </v-btn>
            </v-list-item-action>
          </v-list-item>
        </v-list>
      </v-col>
    </v-row>

    <v-dialog v-model="isDialogShown" scrollable max-width="600px">
      <v-card>
        <v-card-title>{{ dialogTitle }}</v-card-title>
        <v-divider></v-divider>
        <v-card-text style="height: 300px">
          {{ dialogText }}
        </v-card-text>
        <v-divider></v-divider>
        <v-card-actions>
          <v-btn color="blue darken-1" text @click="showDialog('', '', false)">
            Close
          </v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>
  </v-container>
</template>

<script>
// @ is an alias to /src
// import Welcome from "@/components/Welcome.vue";

export default {
  name: "",
  components: {
    // Welcome,
  },
  prop: {},
  model: {},
  data() {
    return {
      isDialogShown: false,
      dialogText: "",
      dialogTitle: "",
      selectedItem: 1,
      executionItems: [
        { key: "Hostname", val: "N/A" },
        { key: "Program Path", val: "N/A" },
        { key: "Process number", val: "N/A" },
        { key: "Thread number", val: "N/A" },
        { key: "Total runtime", val: "N/A" },
        { key: "Memory consumption", val: "N/A" },
        { key: "Process map file", val: "N/A", extra: "N/A" },
        { key: "Environemtal variables", val: "N/A", extra: "N/A" },
      ],
      configItems: [
        { key: "Debug Log", val: "N/A" },
        { key: "Error Log", val: "N/A" },
        { key: "Execution Mode", val: "N/A" },
        { key: "Gcc path", val: "N/A" }
      ],
    };
  },
  methods: {
    showDialog: function (title, content, shouldShow) {
      this.dialogTitle = title;
      this.dialogText = content;
      this.isDialogShown = shouldShow;
    },
  },
};
</script>
