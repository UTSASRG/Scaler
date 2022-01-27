<template>
  <v-container>
    <v-row v-for="(elfItem, itemIndex) in elfList" :key="elfItem.elfName">
      <v-col xl="12">
        <v-list subheader three-line>
          <v-subheader class="text-h4">{{ elfItem.elfName }}</v-subheader>

          <v-list-item
            v-for="elfConfigItem in elfItem.config"
            :key="elfConfigItem.key"
          >
            <v-list-item-content>
              <v-list-item-title>{{ elfConfigItem.key }}</v-list-item-title>
              <v-list-item-subtitle>{{
                elfConfigItem.value
              }}</v-list-item-subtitle>
            </v-list-item-content>
            <v-list-item-action v-if="elfConfigItem.extra">
              <v-btn
                icon
                @click="
                  showDialog(elfConfigItem.key, elfConfigItem.extra, true)
                "
              >
                <v-icon color="grey lighten-1">mdi-information</v-icon>
              </v-btn>
            </v-list-item-action>
          </v-list-item>
        </v-list>
      </v-col>
      <v-col xl="12">
        <v-data-table
          :headers="elfSymInfoHeaders"
          :items="getELFTableData(itemIndex)"
          :single-expand="singleExpand"
          :expanded.sync="expanded"
          :server-items-length="elfItem.totalSymNumber"
          item-key="scalerId"
          @pagination="updateSymbol($event, itemIndex)"
          class="elevation-1"
        >
        </v-data-table>
      </v-col>
    </v-row>
  </v-container>
</template>

<script>
// @ is an alias to /src
// import Welcome from "@/components/Welcome.vue";
import path from "path";
import axios from "axios";
import { scalerConfig } from "../../../../scalerconfig.js";

export default {
  name: "",
  components: {
    // Welcome,
  },
  data() {
    return {
      expanded: [],
      singleExpand: false,
      pageLoaded: false, //When this is ture, pagination won't trigger an update on the server, thus sva
      elfSymInfoHeaders: [
        { text: "ScalerId", value: "scalerId" },
        { text: "Name", value: "symbolName" },
        { text: "Symbol Type", value: "symbolType" },
        { text: "Bind Type", value: "bindType" },
        { text: "Library Id", value: "libFileId" },
        { text: "Got addr", value: "gotAddr" },
        { text: "Hooked", value: "hooked" },
      ],
      desserts: [],
      elfList: [
        // {
        //   elfName: "test.so",
        //   scalerId: 0,
        //   config: [
        //     { key: "File Name", value: "test.so" },
        //     { key: "File Path", value: "N/A" },
        //     { key: "Addr Start", value: "N/A" },
        //     { key: "Addr End", value: "N/A" },
        //     { key: ".plt Start Addr", value: "N/A" },
        //     { key: ".plt.sec Start Addr", value: "N/A" },
        //   ],
        // },{
        //   elfName: "test1.so",
        //   scalerId: 1,
        //   config: [
        //     { key: "File Name", value: "test1.so" },
        //     { key: "File Path", value: "N/A" },
        //     { key: "Addr Start", value: "N/A" },
        //     { key: "Addr End", value: "N/A" },
        //     { key: ".plt Start Addr", value: "N/A" },
        //     { key: ".plt.sec Start Addr", value: "N/A" },
        //   ],
        // },
      ],
      symbolList: [
        // [
        //   {
        //     scalerId: "0",
        //     symbolName: "N/A",
        //     symbolType: "N/A",
        //     bindType: "N/A",
        //     callerFileId: "N/A",
        //     symIdInFile: "N/A",
        //     libFileId: "N/A",
        //     gotAddr: "N/A",
        //     hooked: "N/A",
        //   },
        //   {
        //     scalerId: "1",
        //     symbolName: "N/A",
        //     symbolType: "N/A",
        //     bindType: "N/A",
        //     callerFileId: "N/A",
        //     symIdInFile: "N/A",
        //     libFileId: "N/A",
        //     gotAddr: "N/A",
        //     hooked: "N/A",
        //   },
        // ],
        // [
        //   {
        //     scalerId: "0",
        //     symbolName: "N/A",
        //     symbolType: "N/A",
        //     bindType: "N/A",
        //     callerFileId: "N/A",
        //     symIdInFile: "N/A",
        //     libFileId: "N/A",
        //     gotAddr: "N/A",
        //     hooked: "N/A",
        //   },
        //   {
        //     scalerId: "1",
        //     symbolName: "N/A",
        //     symbolType: "N/A",
        //     bindType: "N/A",
        //     callerFileId: "N/A",
        //     symIdInFile: "N/A",
        //     libFileId: "N/A",
        //     gotAddr: "N/A",
        //     hooked: "N/A",
        //   },
        // ]
      ],
    };
  },
  props: ["jobid"],
  model: {},
  methods: {
    showDialog: function (title, content, shouldShow) {
      this.dialogTitle = title;
      this.dialogText = content;
      this.isDialogShown = shouldShow;
    },
    getELFTableData: function (elfID) {
      return this.symbolList[elfID];
    },
    updateSymbol(pagingInfo, elfItemIndex) {
      if (this.pageLoaded) {
        var thiz = this;
        // Data is not enough, requesting more
        console.log("Data is not enough, requesting more");
        axios
          .get(
            scalerConfig.$ANALYZER_SERVER_URL +
              "/elfInfo/symbol?jobid=" +
              this.jobid +
              "&elfImgId=" +
              this.elfList[elfItemIndex].nodeId +
              "&symPagingStart=" +
              pagingInfo.pageStart +
              "&symPagingNum=" +
              pagingInfo.itemsPerPage
          )
          .then(function (response) {
            var curElfSymList = thiz.symbolList[elfItemIndex];
            curElfSymList.splice(0);

            for (var j = 0; j < response.data.length; j++) {
              var curSym = response.data[j];

              curElfSymList.push({
                scalerId: curSym.scalerId,
                symbolName: curSym.symbolName,
                symbolType: curSym.symbolType,
                bindType: curSym.bindType,
                callerFileId: curSym.callerFileId,
                symIdInFile: "N/A",
                libFileId: curSym.libFileId,
                gotAddr: curSym.gotAddr,
                hooked: curSym.hooked,
              });
            }
            // thiz.symbolList[elfItemIndex] = curElfSymList;
            // console.log(thiz.symbolList[elfItemIndex])
          });
      }
    },
  },
  mounted: function () {
    let thiz = this;
    //Getting ELFimgInfo from the server
    axios
      .get(
        scalerConfig.$ANALYZER_SERVER_URL +
          "/elfInfo?jobid=" +
          this.jobid +
          "&symPagingNum=10"
      )
      .then(function (response) {
        for (var i = 0; i < response.data.length; i++) {
          var curImg = response.data[i].curImg;

          if (curImg.elfImgValid) {
            //Adding elfImgInfo
            thiz.elfList.push({
              elfName: path.basename(curImg.filePath),
              nodeId: curImg.id,
              scalerId: 0,
              totalSymNumber: response.data[i].totalSymNumber,
              config: [
                { key: "File Name", value: path.basename(curImg.filePath) },
                { key: "File Path", value: curImg.filePath },
                { key: "Addr Start", value: "N/A" },
                { key: "Addr End", value: "N/A" },
                { key: ".plt Start Addr", value: curImg.pltStartAddr },
                { key: ".plt.sec Start Addr", value: curImg.pltSecStartAddr },
              ],
            });
            var curElfSymList = [];
            //Adding elfSymInfo
            for (var j = 0; j < curImg.symbolsInThisFile.length; j++) {
              var curSym = curImg.symbolsInThisFile[j];
              curElfSymList.push({
                scalerId: curSym.scalerId,
                symbolName: curSym.symbolName,
                symbolType: curSym.symbolType,
                bindType: curSym.bindType,
                callerFileId: curSym.callerFileId,
                symIdInFile: "N/A",
                libFileId: curSym.libFileId,
                gotAddr: curSym.gotAddr,
                hooked: curSym.hooked,
              });
            }

            thiz.symbolList.push(curElfSymList);
          }
        }
        thiz.pageLoaded=true;
      });
  },
};
</script>
