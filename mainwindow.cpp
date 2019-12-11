#include "mainwindow.h"
#include <QTreeWidget>
#include "ui_mainwindow.h"
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);


    QStringList headers;
    //ui->tableWidget->horizontalHeader()->
    headers << tr("No.") << tr("Offset") << tr("Length") << tr("Start Code") << tr("NAL Type") << tr("Info");
    ui->tableWidget->setColumnCount(headers.size());
    ui->tableWidget->setHorizontalHeaderLabels(headers);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);

}

MainWindow::~MainWindow()
{
    delete ui;
}

std::string MainWindow::video_info_to_string(videoinfo_t *videoInfo) {
    std::string strProfileInfo;
    char szLevelInfo[255];
    char szTierInfo[255];

    // H.265
    if (videoInfo->type)
    {
        // profile����
        switch (videoInfo->profile_idc)
        {
            case PROFILE_NONE:
                strProfileInfo = ("None");
                break;
            case PROFILE_MAIN:
                strProfileInfo = ("Main");
                break;
            case PROFILE_MAIN10:
                strProfileInfo = ("Main10");
                break;
            case PROFILE_MAINSTILLPICTURE:
                strProfileInfo = ("Main Still Picture");
                break;
            case PROFILE_MAINREXT:
                strProfileInfo = ("Main RExt");
                break;
            case PROFILE_HIGHTHROUGHPUTREXT:
                strProfileInfo = ("High Throughput RExt");
                break;
            default:
                strProfileInfo = ("Unkown");
                break;
        }
        switch (videoInfo->level_idc)
        {
            case LEVEL_NONE:
                sprintf(szLevelInfo, "none(%d)", LEVEL_NONE);
                break;
            case LEVEL1:
                sprintf(szLevelInfo, ("1(%d)"), LEVEL1);
                break;
            case LEVEL2:
                sprintf(szLevelInfo, ("2(%d)"), LEVEL2);
                break;
            case LEVEL2_1:
                sprintf(szLevelInfo, ("2.1(%d)"), LEVEL2_1);
                break;
            case LEVEL3:
                sprintf(szLevelInfo, ("3(%d)"), LEVEL3);
                break;
            case LEVEL3_1:
                sprintf(szLevelInfo, ("3.1(%d)"), LEVEL3_1);
                break;
            case LEVEL4:
                sprintf(szLevelInfo, ("4(%d)"), LEVEL4);
                break;
            case LEVEL4_1:
                sprintf(szLevelInfo, ("4.1(%d)"), LEVEL4_1);
                break;
            case LEVEL5:
                sprintf(szLevelInfo, ("5(%d)"), LEVEL5);
                break;
            case LEVEL5_1:
                sprintf(szLevelInfo, ("5.1(%d)"), LEVEL5_1);
                break;
            case LEVEL5_2:
                sprintf(szLevelInfo, ("5.2(%d)"), LEVEL5_2);
                break;
            case LEVEL6:
                sprintf(szLevelInfo, ("6(%d)"), LEVEL6);
                break;
            case LEVEL6_1:
                sprintf(szLevelInfo, ("6.1(%d)"), LEVEL6_1);
                break;
            case LEVEL6_2:
                sprintf(szLevelInfo, ("6.2(%d)"), LEVEL6_2);
                break;
            case LEVEL8_5:
                sprintf(szLevelInfo, ("8.5(%d)"), LEVEL8_5);
                break;
            default:
                sprintf(szLevelInfo, ("Unkown"));
                break;
        }
        switch (videoInfo->tier_idc)
        {
            case 1:
                strcpy(szTierInfo, ("Tier High"));
                break;
            case 0:
            default:
                strcpy(szTierInfo, ("Tier Main"));
                break;
        }
    }
    else // h264
    {
        // profile����
        switch (videoInfo->profile_idc)
        {
            case 66:
                strProfileInfo = ("Baseline");
                break;
            case 77:
                strProfileInfo = ("Main");
                break;
            case 88:
                strProfileInfo = ("Extended");
                break;
            case 100:
                strProfileInfo = ("High");
                break;
            case 110:
                strProfileInfo = ("High 10");
                break;
            case 122:
                strProfileInfo = ("High 422");
                break;
            case 144:
                strProfileInfo = ("High 444");
                break;
            default:
                strProfileInfo = ("Unkown");
                break;
        }

        sprintf(szLevelInfo, ("%d"), videoInfo->level_idc);
    }
    // common
    char szBitDepth[255];
    char *szVideoFormat;
    // bit depth
    sprintf(szBitDepth, ("Luma bit: %d Chroma bit: %d"), videoInfo->bit_depth_luma, videoInfo->bit_depth_chroma);

    // chroma format
    switch (videoInfo->chroma_format_idc)
    {
        case 1:
            szVideoFormat = ("YUV420");
            break;
        case 2:
            szVideoFormat = ("YUV422");
            break;
        case 3:
            szVideoFormat = ("YUV444");
            break;
        case 0:
            szVideoFormat = ("monochrome");
            break;
        default:
            szVideoFormat = ("Unkown");
            break;
    }

    /*
    "Video Format: xxx\r\n"
    */
    char szSimpleInfo[512];
    sprintf(szSimpleInfo,
            "%s File Information\r\n\r\n"
            "Picture Size \t: %dx%d\r\n"
            "  - Cropping Left \t: %d\r\n"
            "  - Cropping Right \t: %d\r\n"
            "  - Cropping Top \t: %d\r\n"
            "  - Cropping Bottom \t: %d\r\n"
            "Video Format \t: %s %s\r\n"
            "Stream Type \t: %s Profile @ Level %s %s\r\n"
            "Encoding Type \t: %s\r\n"
            "Max fps \t\t: %.03f\r\n"
            "Frame Count \t: %d\r\n",
            videoInfo->type ? "H.265/HEVC" : "H.264/AVC",
            videoInfo->width, videoInfo->height,
            videoInfo->crop_left, videoInfo->crop_right,
            videoInfo->crop_top, videoInfo->crop_bottom,
            szVideoFormat, szBitDepth,
            strProfileInfo.c_str(), szLevelInfo, szTierInfo,
            videoInfo->encoding_type ? "CABAC" : "CAVLC",
            videoInfo->max_framerate,0);

    return szSimpleInfo;
}

void MainWindow::add_nalu(NALU_t *nal) {
    char szNalType[255];
    char szNalInfo[255];
    int slice_index = 0;
    if (0 == nal->type) {
        switch (nal->nalType)
        {
            case 0:
                sprintf(szNalType,("Unspecified"));
                break;
            case 1:
                sprintf(szNalType,("Coded slice of a non-IDR picture"));
                switch (nal->sliceType)
                {
                    case 0:
                    case 5:
                        sprintf(szNalInfo,("P Slice #%d"), slice_index);
                        break;
                    case 1:
                    case 6:
                        sprintf(szNalInfo,("B Slice #%d"), slice_index);
                        break;
                    case 2:
                    case 7:
                        sprintf(szNalInfo,("I Slice #%d"), slice_index);
                        break;
                }
                slice_index++;
                break;
            case 2:
                sprintf(szNalType,("DPA"));
                break;
            case 3:
                sprintf(szNalType,("DPB"));
                break;
            case 4:
                sprintf(szNalType,("DPC"));
                break;
            case 5:
                sprintf(szNalType,("Coded slice of an IDR picture"));
                sprintf(szNalInfo,("IDR #%d"), slice_index);
                slice_index++;
                break;
            case 6:
                sprintf(szNalType,("Supplemental enhancement information"));
                sprintf(szNalInfo,("SEI"));
                break;
            case 7:
                sprintf(szNalType,("Sequence parameter set"));
                sprintf(szNalInfo,("SPS"));
                break;
            case 8:
                sprintf(szNalType,("Picture parameter set"));
                sprintf(szNalInfo,("PPS"));
                break;
            case 9:
                sprintf(szNalType,("Access UD"));
                sprintf(szNalInfo,("AUD"));
                break;
            case 10:
                sprintf(szNalType,("END_SEQUENCE"));
                break;
            case 11:
                sprintf(szNalType,("END_STREAM"));
                break;
            case 12:
                sprintf(szNalType,("FILLER_DATA"));
                break;
            case 13:
                sprintf(szNalType,("SPS_EXT"));
                break;
            case 19:
                sprintf(szNalType,("AUXILIARY_SLICE"));
                break;
            default:
                sprintf(szNalType,("Other"));
                break;
        }
    }else{
        switch (nal->nalType)
        {
            // to confirm type...
            case NAL_UNIT_CODED_SLICE_TRAIL_N:
            case NAL_UNIT_CODED_SLICE_TRAIL_R:
                sprintf(szNalType,("Coded slice segment of a non-TSA, non-STSA trailing picture"));
                switch (nal->sliceType)
                {
                    case H265_SH_SLICE_TYPE_B:
                        sprintf(szNalInfo,("B Slice #%d"), slice_index);
                        break;
                    case H265_SH_SLICE_TYPE_P:
                        sprintf(szNalInfo,("P Slice #%d"), slice_index);
                        break;
                    case H265_SH_SLICE_TYPE_I:
                        sprintf(szNalInfo,("I Slice #%d"), slice_index);
                        break;
                }
                slice_index++;
                break;
            case NAL_UNIT_CODED_SLICE_TSA_N:
            case NAL_UNIT_CODED_SLICE_TSA_R:
                sprintf(szNalType,("Coded slice segment of a TSA picture"));
                switch (nal->sliceType)
                {
                    case H265_SH_SLICE_TYPE_B:
                        sprintf(szNalInfo,("B Slice #%d"), slice_index);
                        break;
                    case H265_SH_SLICE_TYPE_P:
                        sprintf(szNalInfo,("P Slice #%d"), slice_index);
                        break;
                    case H265_SH_SLICE_TYPE_I:
                        sprintf(szNalInfo,("I Slice #%d"), slice_index);
                        break;
                }
                slice_index++;
                break;
            case NAL_UNIT_CODED_SLICE_RADL_N:
            case NAL_UNIT_CODED_SLICE_RADL_R:
                sprintf(szNalType,("Coded slice segment of a TSA picture"));
                switch (nal->sliceType)
                {
                    case H265_SH_SLICE_TYPE_B:
                        sprintf(szNalInfo,("B Slice #%d"), slice_index);
                        break;
                    case H265_SH_SLICE_TYPE_P:
                        sprintf(szNalInfo,("P Slice #%d"), slice_index);
                        break;
                    case H265_SH_SLICE_TYPE_I:
                        sprintf(szNalInfo,("I Slice #%d"), slice_index);
                        break;
                }
                slice_index++;
                break;
            case NAL_UNIT_CODED_SLICE_IDR_W_RADL:
            case NAL_UNIT_CODED_SLICE_IDR_N_LP:
                sprintf(szNalType,("Coded slice of an IDR picture"));
                sprintf(szNalInfo,("IDR #%d"), slice_index);
                slice_index++;
                break;
            case NAL_UNIT_CODED_SLICE_CRA:
                sprintf(szNalType,("Coded slice segment of a CRA picture"));
                sprintf(szNalInfo,("CRA #%d"), slice_index);
                slice_index++;
                break;
            case NAL_UNIT_PREFIX_SEI:
            case NAL_UNIT_SUFFIX_SEI:
                sprintf(szNalType,("Supplemental enhancement information"));
                sprintf(szNalInfo,("SEI"));
                break;
            case NAL_UNIT_VPS:
                sprintf(szNalType,("Video parameter set"));
                sprintf(szNalInfo,("VPS"));
                break;
            case NAL_UNIT_SPS:
                sprintf(szNalType,("Sequence parameter set"));
                sprintf(szNalInfo,("SPS"));
                break;
            case NAL_UNIT_PPS:
                sprintf(szNalType,("Picture parameter set"));
                sprintf(szNalInfo,("PPS"));
                break;
            case NAL_UNIT_AUD:
                sprintf(szNalType,("Access UD"));
                sprintf(szNalInfo,("AUD"));
                break;
            case NAL_UNIT_EOS:
                sprintf(szNalType,("END_SEQUENCE"));
                break;
            case NAL_UNIT_EOB:
                sprintf(szNalType,("END_STREAM"));
                break;
            case NAL_UNIT_FILLER_DATA:
                sprintf(szNalType,("FILLER_DATA"));
                break;
            default:
                sprintf(szNalType,("Unknown"));
                break;
        }
    }

    int row = ui->tableWidget->rowCount();
    ui->tableWidget->insertRow(row);
    ui->tableWidget->setItem(row, 0, new QTableWidgetItem(QString::number(nal->num + 1)));
    ui->tableWidget->setItem(row, 1, new QTableWidgetItem(QString::number(nal->offset)));
    ui->tableWidget->setItem(row, 2, new QTableWidgetItem(QString::number(nal->len)));
    ui->tableWidget->setItem(row, 3, new QTableWidgetItem(QString::fromLatin1(nal->startcodeBuffer)));
    ui->tableWidget->setItem(row, 4, new QTableWidgetItem(QString::fromLatin1(szNalType)));
    ui->tableWidget->setItem(row, 5, new QTableWidgetItem(QString::fromLatin1(szNalInfo)));

}

void MainWindow::on_actionOpen_File_triggered()
{
    QFileDialog *fileDialog = new QFileDialog(this);
    fileDialog->setWindowTitle(tr("Open File"));
    fileDialog->setNameFilter(tr("stream file(*.*)"));
    fileDialog->setFileMode(QFileDialog::ExistingFiles);
    QStringList selectedFiles;
    if (fileDialog->exec()) {
        selectedFiles = fileDialog->selectedFiles();
    }else {
        return;
    }

    if (selectedFiles.size() == 0) return;

    m_parser.init(selectedFiles.at(0).toStdString().c_str(), ui->treeWidget);

    m_parser.probeNALU(m_vctNalu, 100);
    for(int i = 0; i < m_vctNalu.size(); ++i) {
        add_nalu(&m_vctNalu[i]);
    }

    ui->tableWidget->resizeColumnsToContents();

    videoinfo_t videoInfo;
    m_parser.getVideoInfo(&videoInfo);

    ui->textBrowser->setText(QString::fromStdString(video_info_to_string(&videoInfo)));
}

void MainWindow::on_actionAbout_triggered()
{

}

void MainWindow::on_tableWidget_itemClicked(QTableWidgetItem *item)
{
    int row = item->row();
    char *nal_data,  *nal_info;
    m_parser.parseNALU(m_vctNalu[row], &nal_data, &nal_info);
    QString str = nal_info;
    QByteArray qa;
    str += qa.append(nal_data, m_vctNalu[row].len).toHex();

    ui->plainTextEdit->setPlainText(str);
}
