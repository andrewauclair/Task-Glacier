package tree;

import net.byteseek.swing.treetable.TreeTableHeaderRenderer;
import net.byteseek.swing.treetable.TreeTableModel;
import packets.DailyReportMessage;

import javax.swing.*;
import javax.swing.table.TableCellRenderer;
import javax.swing.tree.DefaultMutableTreeNode;
import javax.swing.tree.DefaultTreeModel;
import javax.swing.tree.TreeNode;
import java.awt.*;

public class DailyReportTreeTable extends JTable {
    private final DefaultMutableTreeNode rootNode = new DefaultMutableTreeNode();
    private final TreeTableModel treeTableModel;
    private final DefaultTreeModel treeModel;

    private final ReportBuilder reportBuilder;

    public DailyReportTreeTable() {

        treeTableModel = createTreeTableModel(rootNode);
        treeModel = createTreeModel(rootNode);

        reportBuilder = new ReportBuilder(rootNode, treeTableModel, treeModel);

        getColumnModel().getColumn(1).setCellRenderer(new ElapsedTimeCellRenderer());

        setIntercellSpacing(new Dimension(0, 0));
    }

    @Override
    public Component prepareRenderer(TableCellRenderer renderer, int row, int column) {
        JLabel label = (JLabel) super.prepareRenderer(renderer, row, column);

        int totalsRowIndex = ((DailyReportTreeTableModel) treeTableModel).getFirstTotalsRowIndex();

        if (totalsRowIndex != 0 && totalsRowIndex == row) {
            label.setBorder(BorderFactory.createCompoundBorder(BorderFactory.createMatteBorder(2, 0, 0, 0, UIManager.getColor("Component.borderColor")), label.getBorder()));
        }

        return label;
    }

    public void update(DailyReportMessage.DailyReport report) {
        reportBuilder.updateForDailyReport(report);
    }

    private DefaultTreeModel createTreeModel(TreeNode rootNode) {
        DefaultTreeModel model = new DefaultTreeModel(rootNode);
        model.addTreeModelListener(treeTableModel);
        return model;
    }

    private TreeTableModel createTreeTableModel(TreeNode rootNode) {
        TreeTableModel localTreeTableModel = new DailyReportTreeTableModel(rootNode);

        TreeTableHeaderRenderer renderer = new TreeTableHeaderRenderer();
//        renderer.setShowNumber(true); // true is default, this code is just for testing the false option.

        localTreeTableModel.bindTable(this, renderer);
        localTreeTableModel.addExpandCollapseListener(new TreeTableModel.ExpandCollapseListener() {
            @Override
            public boolean nodeExpanding(TreeNode node) {
                if (node.getChildCount() == 0) { // if a node is expanding but has no children, set it to allow no children.
                    ((DefaultMutableTreeNode) node).setAllowsChildren(false);
                }
                return true;
            }

            @Override
            public boolean nodeCollapsing(TreeNode node) {
                return true;
            }
        });
        return localTreeTableModel;
    }
}
